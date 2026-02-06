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
                const std::string& symbol)
        : Strategy(name, portfolio)
        , symbol_(symbol)
        , fastPeriod_(Config::getInt("strategy.sma.fast_period", 10))
        , slowPeriod_(Config::getInt("strategy.sma.slow_period", 30))
        , positionSize_(Config::getInt("strategy.sma.position_size", 10000))
        , previousCross_(CrossState::NONE)
    {}

    /**
     * Constructor with explicit parameters (overrides config)
     */
    SMAStrategy(const std::string& name, 
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

    /**
     * Get the symbol this strategy trades
     */
    const std::string& getSymbol() const {
        return symbol_;
    }

    /**
     * Get fast SMA (returns 0 if not enough data)
     */
    double getFastSMA() const {
        return calculateSMA(prices_, fastPeriod_);
    }

    /**
     * Get slow SMA (returns 0 if not enough data)
     */
    double getSlowSMA() const {
        return calculateSMA(prices_, slowPeriod_);
    }

    /**
     * Get number of price points collected
     */
    size_t getPriceCount() const {
        return prices_.size();
    }

protected:
    void onStart() override {
        // Initialize strategy state
        prices_.clear();
        previousCross_ = CrossState::NONE;
    }

    void onStop() override {
        // Cleanup if needed
    }

    void onTradeEvent(const TradeEvent& event) override {
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

    void onFillEvent(const FillEvent& event) override {
        // Track fills for this strategy
        if (event.getSymbol() == symbol_) {
            // Could log or track fill information here
        }
    }

private:
    enum class CrossState {
        NONE,          // Not initialized
        FAST_ABOVE,    // Fast SMA above slow SMA
        FAST_BELOW     // Fast SMA below slow SMA
    };

    /**
     * Calculate simple moving average over the last 'period' prices
     */
    static double calculateSMA(const std::deque<double>& prices, int period) {
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

    std::string symbol_;
    int fastPeriod_;
    int slowPeriod_;
    int64_t positionSize_;
    
    std::deque<double> prices_;  // Price history
    CrossState previousCross_;   // Previous cross state for detecting changes
};

} // namespace engine
