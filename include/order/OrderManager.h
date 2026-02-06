#pragma once

#include "event/EventBus.h"
#include "order/Order.h"
#include "order/Position.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

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
    OrderManager() {
        // Subscribe to order lifecycle events
        orderSubId_ = EventBus::getInstance().subscribe(
            EventType::ORDER,
            [this](const Event& event) { this->onOrderEvent(event); }
        );
        
        // Subscribe to fill events
        fillSubId_ = EventBus::getInstance().subscribe(
            EventType::FILL,
            [this](const Event& event) { this->onFillEvent(event); }
        );
    }
    
    ~OrderManager() {
        EventBus::getInstance().unsubscribe(orderSubId_);
        EventBus::getInstance().unsubscribe(fillSubId_);
    }

    // Prevent copying
    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;

    /**
     * Submit a new order (publishes ORDER event with PENDING_NEW status)
     */
    void submitOrder(const std::string& orderId, const std::string& symbol,
                     Side side, OrderType type, double price, int64_t quantity) {
        // Create order object and store it (with lock)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto order = std::make_shared<Order>(orderId, symbol, side, type, price, quantity);
            orders_[orderId] = order;
        }
        
        // Publish order event (without lock to avoid deadlock)
        OrderEvent event(orderId, symbol, side, type, OrderStatus::PENDING_NEW, 
                        price, quantity);
        EventBus::getInstance().publish(event);
    }

    /**
     * Request to cancel an order
     */
    void cancelOrder(const std::string& orderId) {
        // Build cancel event (with lock)
        OrderEvent event("", "", Side::BUY, OrderType::LIMIT, OrderStatus::PENDING_CANCEL, 0.0, 0);
        bool shouldCancel = false;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = orders_.find(orderId);
            if (it != orders_.end() && it->second->isActive()) {
                event = OrderEvent(orderId, it->second->getSymbol(),
                                 it->second->getSide(), it->second->getType(),
                                 OrderStatus::PENDING_CANCEL,
                                 it->second->getLimitPrice(), 
                                 it->second->getQuantity(),
                                 it->second->getFilledQuantity());
                shouldCancel = true;
            }
        }
        
        // Publish outside lock to avoid deadlock
        if (shouldCancel) {
            EventBus::getInstance().publish(event);
        }
    }

    /**
     * Get order by ID
     */
    std::shared_ptr<Order> getOrder(const std::string& orderId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(orderId);
        return (it != orders_.end()) ? it->second : nullptr;
    }

    /**
     * Get all active orders
     */
    std::vector<std::shared_ptr<Order>> getActiveOrders() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Order>> active;
        
        for (const auto& [id, order] : orders_) {
            if (order->isActive()) {
                active.push_back(order);
            }
        }
        
        return active;
    }

    /**
     * Get all active orders for a specific symbol
     */
    std::vector<std::shared_ptr<Order>> getActiveOrdersForSymbol(const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Order>> active;
        
        for (const auto& [id, order] : orders_) {
            if (order->isActive() && order->getSymbol() == symbol) {
                active.push_back(order);
            }
        }
        
        return active;
    }

    /**
     * Get position for a symbol (returns nullptr if no position)
     */
    std::shared_ptr<Position> getPosition(const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = positions_.find(symbol);
        return (it != positions_.end()) ? it->second : nullptr;
    }

    /**
     * Get all positions
     */
    std::vector<std::shared_ptr<Position>> getAllPositions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Position>> result;
        
        for (const auto& [symbol, position] : positions_) {
            if (!position->isFlat()) {
                result.push_back(position);
            }
        }
        
        return result;
    }

    /**
     * Get total realized P&L across all positions
     */
    double getTotalRealizedPnL() const {
        std::lock_guard<std::mutex> lock(mutex_);
        double total = 0.0;
        
        for (const auto& [symbol, position] : positions_) {
            total += position->getRealizedPnL();
        }
        
        return total;
    }

    /**
     * Get total unrealized P&L (requires current market prices)
     */
    double getTotalUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const {
        std::lock_guard<std::mutex> lock(mutex_);
        double total = 0.0;
        
        for (const auto& [symbol, position] : positions_) {
            auto priceIt = marketPrices.find(symbol);
            if (priceIt != marketPrices.end()) {
                total += position->getUnrealizedPnL(priceIt->second);
            }
        }
        
        return total;
    }

    /**
     * Get count of active orders
     */
    size_t getActiveOrderCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        
        for (const auto& [id, order] : orders_) {
            if (order->isActive()) ++count;
        }
        
        return count;
    }

    /**
     * Clear all orders and positions (useful for testing)
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        orders_.clear();
        positions_.clear();
    }

private:
    /**
     * Handle order status updates
     */
    void onOrderEvent(const Event& event) {
        const auto* orderEvent = dynamic_cast<const OrderEvent*>(&event);
        if (!orderEvent) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find or create order
        auto it = orders_.find(orderEvent->getOrderId());
        if (it != orders_.end()) {
            it->second->updateFromEvent(*orderEvent);
        } else {
            // Order not found - create it (shouldn't happen normally)
            auto order = std::make_shared<Order>(
                orderEvent->getOrderId(),
                orderEvent->getSymbol(),
                orderEvent->getSide(),
                orderEvent->getOrderType(),
                orderEvent->getPrice(),
                orderEvent->getQuantity()
            );
            order->updateFromEvent(*orderEvent);
            orders_[orderEvent->getOrderId()] = order;
        }
    }

    /**
     * Handle fill events and update positions
     */
    void onFillEvent(const Event& event) {
        const auto* fillEvent = dynamic_cast<const FillEvent*>(&event);
        if (!fillEvent) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Update order
        auto orderIt = orders_.find(fillEvent->getOrderId());
        if (orderIt != orders_.end()) {
            orderIt->second->applyFill(fillEvent->getFillQuantity(), 
                                      fillEvent->getFillPrice());
        }
        
        // Update position
        const std::string& symbol = fillEvent->getSymbol();
        auto posIt = positions_.find(symbol);
        
        if (posIt == positions_.end()) {
            // Create new position
            auto position = std::make_shared<Position>(symbol);
            position->applyFill(fillEvent->getSide(), 
                              fillEvent->getFillQuantity(),
                              fillEvent->getFillPrice());
            positions_[symbol] = position;
        } else {
            // Update existing position
            posIt->second->applyFill(fillEvent->getSide(),
                                    fillEvent->getFillQuantity(),
                                    fillEvent->getFillPrice());
        }
    }

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
