#include "order/OrderManager.h"

namespace engine {

OrderManager::OrderManager() {
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

OrderManager::~OrderManager() {
    EventBus::getInstance().unsubscribe(orderSubId_);
    EventBus::getInstance().unsubscribe(fillSubId_);
}

void OrderManager::submitOrder(const std::string& orderId, const std::string& symbol,
                 Side side, OrderType type, double price, int64_t quantity) {
    // Create order object and store it (with lock)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto order = std::make_shared<Order>(orderId, symbol, side, type, price, quantity);
        orders_[orderId] = order;
    }
    
    // Publish PENDING_NEW event
    OrderEvent event(orderId, symbol, side, type, OrderStatus::PENDING_NEW, price, quantity);
    EventBus::getInstance().publish(event);
}

void OrderManager::cancelOrder(const std::string& orderId) {
    // Check if order exists and is active
    std::shared_ptr<Order> order;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(orderId);
        if (it == orders_.end() || !it->second->isActive()) {
            return;  // Order not found or not active
        }
        order = it->second;
    }
    
    // Publish PENDING_CANCEL event
    OrderEvent event(order->getOrderId(), order->getSymbol(), order->getSide(),
                    order->getType(), OrderStatus::PENDING_CANCEL,
                    order->getLimitPrice(), order->getQuantity());
    EventBus::getInstance().publish(event);
}

std::shared_ptr<Order> OrderManager::getOrder(const std::string& orderId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = orders_.find(orderId);
    return (it != orders_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Order>> OrderManager::getActiveOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Order>> active;
    
    for (const auto& [id, order] : orders_) {
        if (order->isActive()) {
            active.push_back(order);
        }
    }
    
    return active;
}

std::vector<std::shared_ptr<Order>> OrderManager::getActiveOrdersForSymbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Order>> active;
    
    for (const auto& [id, order] : orders_) {
        if (order->isActive() && order->getSymbol() == symbol) {
            active.push_back(order);
        }
    }
    
    return active;
}

std::shared_ptr<Position> OrderManager::getPosition(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Position>> OrderManager::getAllPositions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Position>> result;
    
    for (const auto& [symbol, position] : positions_) {
        if (!position->isFlat()) {
            result.push_back(position);
        }
    }
    
    return result;
}

double OrderManager::getTotalRealizedPnL() const {
    std::lock_guard<std::mutex> lock(mutex_);
    double total = 0.0;
    
    for (const auto& [symbol, position] : positions_) {
        total += position->getRealizedPnL();
    }
    
    return total;
}

double OrderManager::getTotalUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const {
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

size_t OrderManager::getActiveOrderCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    
    for (const auto& [id, order] : orders_) {
        if (order->isActive()) ++count;
    }
    
    return count;
}

void OrderManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    orders_.clear();
    positions_.clear();
}

void OrderManager::onOrderEvent(const Event& event) {
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

void OrderManager::onFillEvent(const Event& event) {
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

} // namespace engine
