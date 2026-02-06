#pragma once

#include "config/Config.h"
#include "strategy/Strategy.h"

#include <deque>
#include <string>

namespace engine {

/**
 * SMAStrategy: Simple Moving Average Crossover Strategy
 * 
 * Trading Logic:
 * - Maintains fast and slow simple moving averages (SMA)
 * - BUY signal: Fast SMA crosses above slow SMA (golden cross)
 * - SELL signal: Fast SMA crosses below slow SMA (death cross)
 * - Only trades one position at a time (flips between long/flat/short)
 * 
 * Parameters:
 * - fastPeriod: Period for fast SMA (e.g., 10)
 * - slowPeriod: Period for slow SMA (e.g., 30)
 * - positionSize: Number of shares to trade
 */
class SMAStrategy : public Strategy {
public:
    /**
     * Constructor - loads settings from config
     */
    SMAStrategy(const std::string& name, 
                std::shared_ptr<Portfolio> portfolio,
                const std::string& symbol);

    /**
     * Constructor with explicit parameters (overrides config)
     */
    SMAStrategy(const std::string& name, 
                std::shared_ptr<Portfolio> portfolio,
                const std::string& symbol,
                int fastPeriod,
                int slowPeriod,
                int64_t positionSize);

    /**
     * Get the symbol this strategy trades
     */
    const std::string& getSymbol() const;

    /**
     * Get fast SMA (returns 0 if not enough data)
     */
    double getFastSMA() const;

    /**
     * Get slow SMA (returns 0 if not enough data)
     */
    double getSlowSMA() const;

    /**
     * Get number of price points collected
     */
    size_t getPriceCount() const;

protected:
    void onStart() override;
    void onStop() override;
    void onTradeEvent(const TradeEvent& event) override;
    void onFillEvent(const FillEvent& event) override;

private:
    enum class CrossState {
        NONE,          // Not initialized
        FAST_ABOVE,    // Fast SMA above slow SMA
        FAST_BELOW     // Fast SMA below slow SMA
    };

    /**
     * Calculate simple moving average over the last 'period' prices
     */
    static double calculateSMA(const std::deque<double>& prices, int period);

    std::string symbol_;
    int fastPeriod_;
    int slowPeriod_;
    int64_t positionSize_;
    
    std::deque<double> prices_;  // Price history
    CrossState previousCross_;   // Previous cross state for detecting changes
};

} // namespace engine
