#include "strategy/SMAStrategy.h"

namespace engine {

SMAStrategy::SMAStrategy(const std::string& name, 
            std::shared_ptr<Portfolio> portfolio,
            const std::string& symbol)
    : Strategy(name, portfolio)
    , symbol_(symbol)
    , fastPeriod_(Config::getInt("strategy.sma.fast_period", 10))
    , slowPeriod_(Config::getInt("strategy.sma.slow_period", 30))
    , positionSize_(Config::getInt("strategy.sma.position_size", 10000))
    , previousCross_(CrossState::NONE)
{}

SMAStrategy::SMAStrategy(const std::string& name, 
            std::shared_ptr<Portfolio> portfolio,
            const std::string& symbol,
            int fastPeriod,
            int slowPeriod,
            int64_t positionSize)
    : Strategy(name, portfolio)
    , symbol_(symbol)
    , fastPeriod_(fastPeriod)
    , slowPeriod_(slowPeriod)
    , positionSize_(positionSize)
    , previousCross_(CrossState::NONE)
{}

const std::string& SMAStrategy::getSymbol() const {
    return symbol_;
}

double SMAStrategy::getFastSMA() const {
    return calculateSMA(prices_, fastPeriod_);
}

double SMAStrategy::getSlowSMA() const {
    return calculateSMA(prices_, slowPeriod_);
}

size_t SMAStrategy::getPriceCount() const {
    return prices_.size();
}

void SMAStrategy::onStart() {
    // Initialize strategy state
    prices_.clear();
    previousCross_ = CrossState::NONE;
}

void SMAStrategy::onStop() {
    // Cleanup if needed
}

void SMAStrategy::onTradeEvent(const TradeEvent& event) {
    // Only process data for our symbol
    if (event.getSymbol() != symbol_) {
        return;
    }

    // Add new price to history
    double price = event.getPrice();
    prices_.push_back(price);

    // Keep only what we need for slow SMA
    if (prices_.size() > static_cast<size_t>(slowPeriod_)) {
        prices_.pop_front();
    }

    // Need at least slowPeriod prices to trade
    if (prices_.size() < static_cast<size_t>(slowPeriod_)) {
        return;
    }

    // Calculate SMAs
    double fastSMA = getFastSMA();
    double slowSMA = getSlowSMA();

    if (fastSMA == 0.0 || slowSMA == 0.0) {
        return;
    }

    // Determine current cross state
    CrossState currentCross = (fastSMA > slowSMA) ? CrossState::FAST_ABOVE : CrossState::FAST_BELOW;

    // Check for crossover
    if (previousCross_ != CrossState::NONE && currentCross != previousCross_) {
        // Get current position
        auto position = getPosition(symbol_);
        int64_t currentQty = position ? position->getQuantity() : 0;

        // Golden cross: fast crosses above slow -> GO LONG
        if (currentCross == CrossState::FAST_ABOVE && currentQty <= 0) {
            // Close short if we have one, then go long
            int64_t targetQty = positionSize_;
            int64_t orderQty = targetQty - currentQty;
            
            std::unordered_map<std::string, double> prices = {{symbol_, price}};
            std::string orderId = generateOrderId();
            submitOrder(orderId, symbol_, Side::BUY, OrderType::MARKET, 
                       price, orderQty, prices);
        }
        // Death cross: fast crosses below slow -> GO SHORT
        else if (currentCross == CrossState::FAST_BELOW && currentQty >= 0) {
            // Close long if we have one, then go short
            int64_t targetQty = -positionSize_;
            int64_t orderQty = std::abs(targetQty - currentQty);
            
            std::unordered_map<std::string, double> prices = {{symbol_, price}};
            std::string orderId = generateOrderId();
            submitOrder(orderId, symbol_, Side::SELL, OrderType::MARKET,
                       price, orderQty, prices);
        }
    }

    previousCross_ = currentCross;
}

void SMAStrategy::onFillEvent(const FillEvent& event) {
    // Track fills for this strategy
    if (event.getSymbol() == symbol_) {
        // Could log or track fill information here
    }
}

double SMAStrategy::calculateSMA(const std::deque<double>& prices, int period) {
    if (prices.size() < static_cast<size_t>(period)) {
        return 0.0;
    }

    double sum = 0.0;
    auto it = prices.rbegin();
    for (int i = 0; i < period; ++i, ++it) {
        sum += *it;
    }

    return sum / period;
}

} // namespace engine
