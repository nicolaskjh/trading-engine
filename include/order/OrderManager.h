#pragma once

#include "event/EventBus.h"
#include "order/Order.h"
#include "order/Position.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

/**
 * OrderManager: Central system for order and position management.
 * 
 * Responsibilities:
 * - Track all orders (active and historical)
 * - Maintain positions per symbol
 * - Process order status updates and fills
 * - Provide query interface for orders and positions
 * - Calculate portfolio-level P&L
 */
class OrderManager {
public:
    OrderManager();
    ~OrderManager();

    // Prevent copying
    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;

    /**
     * Submit a new order (publishes ORDER event with PENDING_NEW status)
     */
    void submitOrder(const std::string& orderId, const std::string& symbol,
                     Side side, OrderType type, double price, int64_t quantity);

    /**
     * Request to cancel an order
     */
    void cancelOrder(const std::string& orderId);

    /**
     * Get order by ID
     */
    std::shared_ptr<Order> getOrder(const std::string& orderId) const;

    /**
     * Get all active orders
     */
    std::vector<std::shared_ptr<Order>> getActiveOrders() const;

    /**
     * Get all active orders for a specific symbol
     */
    std::vector<std::shared_ptr<Order>> getActiveOrdersForSymbol(const std::string& symbol) const;

    /**
     * Get position for a symbol (returns nullptr if no position)
     */
    std::shared_ptr<Position> getPosition(const std::string& symbol) const;

    /**
     * Get all positions
     */
    std::vector<std::shared_ptr<Position>> getAllPositions() const;

    /**
     * Get total realized P&L across all positions
     */
    double getTotalRealizedPnL() const;

    /**
     * Get total unrealized P&L (requires current market prices)
     */
    double getTotalUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const;

    /**
     * Get count of active orders
     */
    size_t getActiveOrderCount() const;

    /**
     * Clear all orders and positions (useful for testing)
     */
    void clear();

private:
    /**
     * Handle order status updates
     */
    void onOrderEvent(const Event& event);

    /**
     * Handle fill events and update positions
     */
    void onFillEvent(const Event& event);

    // Order tracking
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_;
    
    // Position tracking
    std::unordered_map<std::string, std::shared_ptr<Position>> positions_;
    
    // Event subscriptions
    uint64_t orderSubId_;
    uint64_t fillSubId_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace engine
