#pragma once

#include "backtesting/HistoricalDataLoader.h"
#include "backtesting/PerformanceMetrics.h"
#include "exchange/SimulatedExchange.h"
#include "risk/Portfolio.h"
#include "strategy/Strategy.h"
#include "strategy/StrategyManager.h"

#include <memory>
#include <string>
#include <vector>

namespace engine {

/**
 * Backtester: Run trading strategies against historical data.
 * 
 * Features:
 * - Replay historical market data through event system
 * - Track portfolio state throughout backtest
 * - Calculate comprehensive performance metrics
 * - Support multiple strategies and symbols
 * 
 * Usage:
 *   Backtester backtester(initialCapital);
 *   backtester.addStrategy(strategy);
 *   backtester.loadData("data.csv");
 *   BacktestResults results = backtester.run();
 */
class Backtester {
public:
    /**
     * Create backtester with initial capital
     */
    explicit Backtester(double initialCapital);
    
    ~Backtester();

    // Prevent copying
    Backtester(const Backtester&) = delete;
    Backtester& operator=(const Backtester&) = delete;

    /**
     * Add a strategy to backtest
     */
    void addStrategy(std::shared_ptr<Strategy> strategy);

    /**
     * Load historical data from CSV file
     */
    void loadData(const std::string& filename);

    /**
     * Load pre-parsed historical data
     */
    void loadData(const std::vector<TradeData>& data);

    /**
     * Set time range for backtest (optional)
     */
    void setTimeRange(int64_t startTime, int64_t endTime);

    /**
     * Set symbols to backtest (optional, defaults to all in data)
     */
    void setSymbols(const std::vector<std::string>& symbols);

    /**
     * Run the backtest
     * Returns comprehensive performance metrics
     */
    BacktestResults run();

    /**
     * Get portfolio snapshots throughout backtest
     */
    const std::vector<PortfolioSnapshot>& getSnapshots() const;

    /**
     * Get portfolio (for inspection after backtest)
     */
    Portfolio* getPortfolio() { return portfolio_.get(); }
    const Portfolio* getPortfolio() const { return portfolio_.get(); }

    /**
     * Clear all state for new backtest
     */
    void reset();

private:
    void replayMarketData();
    void takeSnapshot(int64_t timestamp, double currentPrice);
    BacktestResults calculateResults();

    double initialCapital_;
    std::unique_ptr<Portfolio> portfolio_;
    std::unique_ptr<SimulatedExchange> exchange_;
    std::unique_ptr<StrategyManager> strategyManager_;
    std::vector<std::shared_ptr<Strategy>> strategies_;
    
    std::vector<TradeData> historicalData_;
    std::vector<PortfolioSnapshot> snapshots_;
    
    // Optional filters
    bool hasTimeRange_;
    int64_t startTime_;
    int64_t endTime_;
    std::vector<std::string> symbols_;
};

} // namespace engine
