#include "order/OrderLogger.h"
#include "logger/Logger.h"

#include <iomanip>
#include <sstream>

namespace engine {

OrderLogger::OrderLogger() {
    orderSubId_ = EventBus::getInstance().subscribe(
        EventType::ORDER,
        [this](const Event& event) { this->onOrderEvent(event); }
    );
    
    fillSubId_ = EventBus::getInstance().subscribe(
        EventType::FILL,
        [this](const Event& event) { this->onFillEvent(event); }
    );
    
    Logger::info(LogComponent::ORDER_LOGGER, "Initialized");
}

OrderLogger::~OrderLogger() {
    EventBus::getInstance().unsubscribe(orderSubId_);
    EventBus::getInstance().unsubscribe(fillSubId_);
}

void OrderLogger::onOrderEvent(const Event& event) {
    const auto* order = dynamic_cast<const OrderEvent*>(&event);
    if (!order) return;
    
    std::ostringstream oss;
    oss << "Order " << order->getOrderId()
        << " | " << order->getSymbol()
        << " | " << (order->getSide() == Side::BUY ? "BUY" : "SELL")
        << " | Status: ";
    
    switch (order->getStatus()) {
        case OrderStatus::PENDING_NEW: 
            oss << "PENDING_NEW"; 
            break;
        case OrderStatus::NEW: 
            oss << "NEW (Accepted)"; 
            break;
        case OrderStatus::PARTIALLY_FILLED:
            oss << "PARTIALLY_FILLED (" 
                << order->getFilledQuantity() << "/" 
                << order->getQuantity() << ")";
            break;
        case OrderStatus::FILLED: 
            oss << "FILLED"; 
            break;
        case OrderStatus::CANCELLED: 
            oss << "CANCELLED"; 
            break;
        case OrderStatus::REJECTED: 
            oss << "REJECTED: " << order->getRejectReason(); 
            break;
        default: 
            oss << "UNKNOWN";
    }
    
    oss << " | Latency: " << event.getAgeInMicroseconds() << "μs";
    Logger::info(LogComponent::ORDER_LOGGER, oss.str());
}

void OrderLogger::onFillEvent(const Event& event) {
    const auto* fill = dynamic_cast<const FillEvent*>(&event);
    if (!fill) return;
    
    std::ostringstream oss;
    oss << "Fill for Order " << fill->getOrderId()
        << " | " << fill->getSymbol()
        << " | " << (fill->getSide() == Side::BUY ? "BOUGHT" : "SOLD")
        << " " << fill->getFillQuantity()
        << " @ $" << std::fixed << std::setprecision(2) 
        << fill->getFillPrice()
        << " | Value: $" << std::fixed << std::setprecision(2)
        << (fill->getFillPrice() * fill->getFillQuantity())
        << " | Latency: " << event.getAgeInMicroseconds() << "μs";
    Logger::info(LogComponent::ORDER_LOGGER, oss.str());
}

} // namespace engine
