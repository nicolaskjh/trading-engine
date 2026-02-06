#include "backtesting/Backtester.h"

#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "strategy/StrategyManager.h"

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace engine {

Backtester::Backtester(double initialCapital)
    : initialCapital_(initialCapital)
    , portfolio_(std::make_unique<Portfolio>(initialCapital))
    , strategyManager_(std::make_unique<StrategyManager>())
    , hasTimeRange_(false)
    , startTime_(0)
    , endTime_(0)
{
    // Create exchange with instant fills and no randomness for backtesting
    SimulatedExchange::Config exchangeConfig;
    exchangeConfig.fillLatencyMs = 0;
    exchangeConfig.rejectionRate = 0.0;
    exchangeConfig.partialFillRate = 0.0;
    exchangeConfig.slippageBps = 0.0;
    exchangeConfig.instantFills = true;
    
    exchange_ = std::make_unique<SimulatedExchange>(exchangeConfig);
}

Backtester::~Backtester() = default;

void Backtester::addStrategy(std::shared_ptr<Strategy> strategy) {
    strategies_.push_back(strategy);
    strategyManager_->addStrategy(strategy);
}

void Backtester::loadData(const std::string& filename) {
    historicalData_ = HistoricalDataLoader::loadFromCSV(filename);
}

void Backtester::loadData(const std::vector<TradeData>& data) {
    historicalData_ = data;
}

void Backtester::setTimeRange(int64_t startTime, int64_t endTime) {
    hasTimeRange_ = true;
    startTime_ = startTime;
    endTime_ = endTime;
}

void Backtester::setSymbols(const std::vector<std::string>& symbols) {
    symbols_ = symbols;
}

BacktestResults Backtester::run() {
    if (historicalData_.empty()) {
        throw std::runtime_error("No historical data loaded");
    }
    
    if (strategies_.empty()) {
        throw std::runtime_error("No strategies added");
    }
    
    // Clear previous state
    snapshots_.clear();
    
    // Start exchange and strategies
    exchange_->start();
    strategyManager_->startAll();
    
    // Replay market data
    replayMarketData();
    
    // Stop everything
    strategyManager_->stopAll();
    exchange_->stop();
    
    // Calculate and return results
    return calculateResults();
}

const std::vector<PortfolioSnapshot>& Backtester::getSnapshots() const {
    return snapshots_;
}

void Backtester::reset() {
    snapshots_.clear();
    historicalData_.clear();
    portfolio_ = std::make_unique<Portfolio>(initialCapital_);
    
    SimulatedExchange::Config exchangeConfig;
    exchangeConfig.fillLatencyMs = 0;
    exchangeConfig.rejectionRate = 0.0;
    exchangeConfig.partialFillRate = 0.0;
    exchangeConfig.slippageBps = 0.0;
    exchangeConfig.instantFills = true;
    
    exchange_ = std::make_unique<SimulatedExchange>(exchangeConfig);
}

void Backtester::replayMarketData() {
    // Apply filters
    std::vector<TradeData> filteredData = historicalData_;
    
    // Filter by time range
    if (hasTimeRange_) {
        filteredData = HistoricalDataLoader::filterByTimeRange(
            filteredData, startTime_, endTime_);
    }
    
    // Filter by symbols
    if (!symbols_.empty()) {
        std::vector<TradeData> symbolFiltered;
        for (const auto& symbol : symbols_) {
            auto symbolData = HistoricalDataLoader::filterBySymbol(filteredData, symbol);
            symbolFiltered.insert(symbolFiltered.end(), symbolData.begin(), symbolData.end());
        }
        filteredData = symbolFiltered;
        HistoricalDataLoader::sortByTimestamp(filteredData);
    }
    
    if (filteredData.empty()) {
        throw std::runtime_error("No data after applying filters");
    }
    
    // Take initial snapshot
    takeSnapshot(filteredData[0].timestamp, 0.0);
    
    // Replay each trade
    for (const auto& trade : filteredData) {
        // Update exchange with current market price
        exchange_->setMarketPrice(trade.symbol, trade.price);
        
        // Publish trade event to strategies
        TradeEvent event(trade.symbol, trade.price, trade.volume);
        EventBus::getInstance().publish(event);
        
        // Give time for event processing (in real backtest, this is instant)
        // but we need to let the event callbacks execute
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        
        // Take snapshot periodically (every trade for now)
        takeSnapshot(trade.timestamp, trade.price);
    }
}

void Backtester::takeSnapshot(int64_t timestamp, double currentPrice) {
    // Build market prices map
    std::unordered_map<std::string, double> marketPrices;
    
    // Get unique symbols from current positions
    auto positions = portfolio_->getOrderManager()->getAllPositions();
    for (const auto& position : positions) {
        // Use last known price or current price
        auto it = std::find_if(historicalData_.rbegin(), historicalData_.rend(),
            [&](const TradeData& td) { return td.symbol == position->getSymbol(); });
        
        if (it != historicalData_.rend()) {
            marketPrices[position->getSymbol()] = it->price;
        }
    }
    
    double portfolioValue = portfolio_->getPortfolioValue(marketPrices);
    double cash = portfolio_->getCash();
    double realizedPnL = portfolio_->getRealizedPnL();
    double unrealizedPnL = portfolio_->getUnrealizedPnL(marketPrices);
    
    snapshots_.emplace_back(timestamp, portfolioValue, cash, realizedPnL, unrealizedPnL);
}

BacktestResults Backtester::calculateResults() {
    return PerformanceMetrics::calculate(snapshots_, initialCapital_);
}

} // namespace engine
