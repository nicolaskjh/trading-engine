#pragma once

#include "event/OrderEvent.h"
#include <string>
#include <chrono>

namespace engine {

/**
 * Order: Represents a single order with complete lifecycle tracking.
 * Maintains order state, quantities, and pricing information.
 */
class Order {
public:
    Order(const std::string& orderId, const std::string& symbol,
          Side side, OrderType type, double price, int64_t quantity)
        : orderId_(orderId),
          symbol_(symbol),
          side_(side),
          type_(type),
          status_(OrderStatus::PENDING_NEW),
          limitPrice_(price),
          quantity_(quantity),
          filledQuantity_(0),
          averageFillPrice_(0.0),
          creationTime_(std::chrono::high_resolution_clock::now()),
          lastUpdateTime_(creationTime_) {}

    // Getters
    const std::string& getOrderId() const { return orderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Side getSide() const { return side_; }
    OrderType getType() const { return type_; }
    OrderStatus getStatus() const { return status_; }
    double getLimitPrice() const { return limitPrice_; }
    int64_t getQuantity() const { return quantity_; }
    int64_t getFilledQuantity() const { return filledQuantity_; }
    int64_t getRemainingQuantity() const { return quantity_ - filledQuantity_; }
    double getAverageFillPrice() const { return averageFillPrice_; }
    const std::string& getRejectReason() const { return rejectReason_; }
    
    auto getCreationTime() const { return creationTime_; }
    auto getLastUpdateTime() const { return lastUpdateTime_; }

    // State queries
    bool isActive() const {
        return status_ == OrderStatus::NEW || 
               status_ == OrderStatus::PARTIALLY_FILLED ||
               status_ == OrderStatus::PENDING_NEW;
    }
    
    bool isFilled() const { return status_ == OrderStatus::FILLED; }
    bool isCancelled() const { return status_ == OrderStatus::CANCELLED; }
    bool isRejected() const { return status_ == OrderStatus::REJECTED; }
    bool isTerminal() const { return isFilled() || isCancelled() || isRejected(); }

    /**
     * Update order status from OrderEvent
     */
    void updateFromEvent(const OrderEvent& event) {
        status_ = event.getStatus();
        filledQuantity_ = event.getFilledQuantity();
        
        if (!event.getRejectReason().empty()) {
            rejectReason_ = event.getRejectReason();
        }
        
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * Apply a fill to this order and update average fill price
     */
    void applyFill(int64_t fillQuantity, double fillPrice) {
        int64_t previousFilled = filledQuantity_;
        filledQuantity_ += fillQuantity;
        
        // Update average fill price using weighted average
        if (previousFilled == 0) {
            averageFillPrice_ = fillPrice;
        } else {
            averageFillPrice_ = 
                (averageFillPrice_ * previousFilled + fillPrice * fillQuantity) / filledQuantity_;
        }
        
        // Update status
        if (filledQuantity_ >= quantity_) {
            status_ = OrderStatus::FILLED;
        } else if (filledQuantity_ > 0) {
            status_ = OrderStatus::PARTIALLY_FILLED;
        }
        
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();
    }

private:
    std::string orderId_;
    std::string symbol_;
    Side side_;
    OrderType type_;
    OrderStatus status_;
    double limitPrice_;
    int64_t quantity_;
    int64_t filledQuantity_;
    double averageFillPrice_;
    std::string rejectReason_;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> creationTime_;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdateTime_;
};

} // namespace engine
