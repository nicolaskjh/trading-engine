#pragma once

#include "config/Config.h"
#include "order/Order.h"
#include "order/OrderManager.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace engine {

/**
 * Portfolio: Risk management and capital tracking wrapper around OrderManager.
 * 
 * Responsibilities:
 * - Track capital (initial capital, cash balance)
 * - Calculate portfolio value (cash + unrealized P&L)
 * - Enforce position limits and risk constraints
 * - Provide pre-trade risk checks
 * - Calculate portfolio-level exposure and metrics
 */
class Portfolio {
public:
    /**
     * Constructor - loads settings from config
     */
    Portfolio();

    /**
     * Constructor with explicit initial capital (overrides config)
     */
    explicit Portfolio(double initialCapital);

    // Prevent copying
    Portfolio(const Portfolio&) = delete;
    Portfolio& operator=(const Portfolio&) = delete;

    /**
     * Submit order with pre-trade risk checks
     * Returns true if order passes risk checks and is submitted
     */
    bool submitOrder(const std::string& orderId, const std::string& symbol,
                    Side side, OrderType type, double price, int64_t quantity,
                    const std::unordered_map<std::string, double>& marketPrices);

    /**
     * Cancel an order
     */
    void cancelOrder(const std::string& orderId);

    /**
     * Get current cash balance
     */
    double getCash() const;

    /**
     * Get initial capital
     */
    double getInitialCapital() const { return initialCapital_; }

    /**
     * Calculate portfolio value (cash + unrealized P&L)
     */
    double getPortfolioValue(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Get realized P&L
     */
    double getRealizedPnL() const;

    /**
     * Get unrealized P&L
     */
    double getUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Get total P&L (realized + unrealized)
     */
    double getTotalPnL(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Calculate current gross exposure (sum of absolute position values)
     */
    double getGrossExposure(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Calculate net exposure (long value - short value)
     */
    double getNetExposure(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Set maximum position size limit per symbol
     */
    void setMaxPositionSize(double maxSize);

    /**
     * Get maximum position size limit
     */
    double getMaxPositionSize() const;

    /**
     * Set maximum portfolio exposure limit
     */
    void setMaxPortfolioExposure(double maxExposure);

    /**
     * Get maximum portfolio exposure limit
     */
    double getMaxPortfolioExposure() const;

    /**
     * Access underlying OrderManager
     */
    OrderManager* getOrderManager() { return orderManager_.get(); }
    const OrderManager* getOrderManager() const { return orderManager_.get(); }

    /**
     * Clear all state (useful for testing)
     */
    void clear();

private:
    /**
     * Pre-trade risk checks
     * Returns true if order passes all risk constraints
     */
    bool preTradeRiskCheck(const std::string& symbol, Side side, double price,
                          int64_t quantity, 
                          const std::unordered_map<std::string, double>& marketPrices) const;

    // Capital tracking
    double initialCapital_;
    double cash_;
    
    // Order and position management
    std::unique_ptr<OrderManager> orderManager_;
    
    // Risk limits
    double maxPositionSize_;
    double maxPortfolioExposure_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace engine
