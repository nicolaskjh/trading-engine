#pragma once

#include "Event.h"
#include <string>

namespace engine {

/**
 * Side: Buy or Sell
 */
enum class Side {
    BUY,
    SELL
};

/**
 * OrderStatus: Lifecycle states of an order
 */
enum class OrderStatus {
    PENDING_NEW,
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    PENDING_CANCEL,
    CANCELLED,
    REJECTED
};

/**
 * OrderType: Different order instructions
 */
enum class OrderType {
    MARKET,         // Execute at best available price
    LIMIT,          // Execute at specified price or better
    STOP,           // Trigger when price reached
    STOP_LIMIT,     // Stop order that becomes limit order
    IOC,            // Immediate or Cancel
    FOK             // Fill or Kill
};

/**
 * OrderEvent: Represents order lifecycle events including
 * status changes, fills, and rejections.
 */
class OrderEvent : public Event {
public:
    OrderEvent(const std::string& orderId, const std::string& symbol,
               Side side, OrderType type, OrderStatus status,
               double price, int64_t quantity, int64_t filledQuantity = 0,
               const std::string& rejectReason = "")
        : Event(EventType::ORDER),
          orderId_(orderId),
          symbol_(symbol),
          side_(side),
          type_(type),
          status_(status),
          price_(price),
          quantity_(quantity),
          filledQuantity_(filledQuantity),
          rejectReason_(rejectReason) {}

    const std::string& getOrderId() const { return orderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Side getSide() const { return side_; }
    OrderType getOrderType() const { return type_; }
    OrderStatus getStatus() const { return status_; }
    double getPrice() const { return price_; }
    int64_t getQuantity() const { return quantity_; }
    int64_t getFilledQuantity() const { return filledQuantity_; }
    int64_t getRemainingQuantity() const { return quantity_ - filledQuantity_; }
    const std::string& getRejectReason() const { return rejectReason_; }
    
    bool isFilled() const { return status_ == OrderStatus::FILLED; }
    bool isActive() const {
        return status_ == OrderStatus::NEW || 
               status_ == OrderStatus::PARTIALLY_FILLED;
    }

private:
    std::string orderId_;
    std::string symbol_;
    Side side_;
    OrderType type_;
    OrderStatus status_;
    double price_;
    int64_t quantity_;
    int64_t filledQuantity_;
    std::string rejectReason_;
};

/**
 * FillEvent: Represents order execution/fill with price and quantity details
 */
class FillEvent : public Event {
public:
    FillEvent(const std::string& orderId, const std::string& symbol,
              Side side, double fillPrice, int64_t fillQuantity,
              const std::string& executionId = "")
        : Event(EventType::FILL),
          orderId_(orderId),
          symbol_(symbol),
          side_(side),
          fillPrice_(fillPrice),
          fillQuantity_(fillQuantity),
          executionId_(executionId) {}

    const std::string& getOrderId() const { return orderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Side getSide() const { return side_; }
    double getFillPrice() const { return fillPrice_; }
    int64_t getFillQuantity() const { return fillQuantity_; }
    const std::string& getExecutionId() const { return executionId_; }

private:
    std::string orderId_;
    std::string symbol_;
    Side side_;
    double fillPrice_;
    int64_t fillQuantity_;
    std::string executionId_;
};

} // namespace engine
