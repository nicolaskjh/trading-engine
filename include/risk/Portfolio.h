#pragma once

#include "order/OrderManager.h"
#include "order/Order.h"

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
     * Constructor with initial capital
     */
    explicit Portfolio(double initialCapital)
        : initialCapital_(initialCapital)
        , cash_(initialCapital)
        , orderManager_(std::make_unique<OrderManager>())
        , maxPositionSize_(1000000)  // Default: $1M per position
        , maxPortfolioExposure_(5000000)  // Default: $5M total exposure
    {}

    // Prevent copying
    Portfolio(const Portfolio&) = delete;
    Portfolio& operator=(const Portfolio&) = delete;

    /**
     * Submit order with pre-trade risk checks
     * Returns true if order passes risk checks and is submitted
     */
    bool submitOrder(const std::string& orderId, const std::string& symbol,
                    Side side, OrderType type, double price, int64_t quantity,
                    const std::unordered_map<std::string, double>& marketPrices) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Pre-trade risk checks
        if (!preTradeRiskCheck(symbol, side, price, quantity, marketPrices)) {
            return false;
        }
        
        // Submit order
        orderManager_->submitOrder(orderId, symbol, side, type, price, quantity);
        return true;
    }

    /**
     * Cancel an order
     */
    void cancelOrder(const std::string& orderId) {
        orderManager_->cancelOrder(orderId);
    }

    /**
     * Get current cash balance
     */
    double getCash() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cash_;
    }

    /**
     * Get initial capital
     */
    double getInitialCapital() const {
        return initialCapital_;
    }

    /**
     * Calculate portfolio value (cash + unrealized P&L)
     */
    double getPortfolioValue(const std::unordered_map<std::string, double>& marketPrices) const {
        std::lock_guard<std::mutex> lock(mutex_);
        double unrealizedPnL = orderManager_->getTotalUnrealizedPnL(marketPrices);
        return cash_ + unrealizedPnL;
    }

    /**
     * Get realized P&L
     */
    double getRealizedPnL() const {
        return orderManager_->getTotalRealizedPnL();
    }

    /**
     * Get unrealized P&L
     */
    double getUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const {
        return orderManager_->getTotalUnrealizedPnL(marketPrices);
    }

    /**
     * Get total P&L (realized + unrealized)
     */
    double getTotalPnL(const std::unordered_map<std::string, double>& marketPrices) const {
        return getRealizedPnL() + getUnrealizedPnL(marketPrices);
    }

    /**
     * Calculate current gross exposure (sum of absolute position values)
     */
    double getGrossExposure(const std::unordered_map<std::string, double>& marketPrices) const {
        std::lock_guard<std::mutex> lock(mutex_);
        double exposure = 0.0;
        
        auto positions = orderManager_->getAllPositions();
        for (const auto& position : positions) {
            auto priceIt = marketPrices.find(position->getSymbol());
            if (priceIt != marketPrices.end()) {
                exposure += std::abs(position->getQuantity() * priceIt->second);
            }
        }
        
        return exposure;
    }

    /**
     * Calculate net exposure (long value - short value)
     */
    double getNetExposure(const std::unordered_map<std::string, double>& marketPrices) const {
        std::lock_guard<std::mutex> lock(mutex_);
        double exposure = 0.0;
        
        auto positions = orderManager_->getAllPositions();
        for (const auto& position : positions) {
            auto priceIt = marketPrices.find(position->getSymbol());
            if (priceIt != marketPrices.end()) {
                exposure += position->getQuantity() * priceIt->second;
            }
        }
        
        return exposure;
    }

    /**
     * Set maximum position size limit per symbol
     */
    void setMaxPositionSize(double maxSize) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxPositionSize_ = maxSize;
    }

    /**
     * Get maximum position size limit
     */
    double getMaxPositionSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxPositionSize_;
    }

    /**
     * Set maximum portfolio exposure limit
     */
    void setMaxPortfolioExposure(double maxExposure) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxPortfolioExposure_ = maxExposure;
    }

    /**
     * Get maximum portfolio exposure limit
     */
    double getMaxPortfolioExposure() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxPortfolioExposure_;
    }

    /**
     * Access underlying OrderManager
     */
    OrderManager* getOrderManager() {
        return orderManager_.get();
    }

    const OrderManager* getOrderManager() const {
        return orderManager_.get();
    }

    /**
     * Clear all state (useful for testing)
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cash_ = initialCapital_;
        orderManager_->clear();
    }

private:
    /**
     * Pre-trade risk checks
     * Returns true if order passes all risk constraints
     */
    bool preTradeRiskCheck(const std::string& symbol, Side side, double price,
                          int64_t quantity, 
                          const std::unordered_map<std::string, double>& marketPrices) const {
        // Calculate order value
        double orderValue = price * quantity;
        
        // Check if we have enough cash for buy orders
        if (side == Side::BUY && orderValue > cash_) {
            return false;  // Insufficient cash
        }
        
        // Calculate what position would be after this order
        auto position = orderManager_->getPosition(symbol);
        int64_t currentQty = position ? position->getQuantity() : 0;
        int64_t newQty = (side == Side::BUY) ? currentQty + quantity : currentQty - quantity;
        double newPositionValue = std::abs(newQty * price);
        
        // Check position size limit
        if (newPositionValue > maxPositionSize_) {
            return false;  // Would exceed position size limit
        }
        
        // Calculate new gross exposure
        double currentExposure = 0.0;
        auto positions = orderManager_->getAllPositions();
        for (const auto& pos : positions) {
            if (pos->getSymbol() == symbol) {
                continue;  // Skip - we'll use new position value
            }
            auto priceIt = marketPrices.find(pos->getSymbol());
            if (priceIt != marketPrices.end()) {
                currentExposure += std::abs(pos->getQuantity() * priceIt->second);
            }
        }
        
        double newExposure = currentExposure + newPositionValue;
        
        // Check portfolio exposure limit
        if (newExposure > maxPortfolioExposure_) {
            return false;  // Would exceed portfolio exposure limit
        }
        
        return true;  // All checks passed
    }

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
