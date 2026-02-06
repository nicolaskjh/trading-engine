#include "exchange/SimulatedExchange.h"

#include <chrono>
#include <random>
#include <thread>

namespace engine {

SimulatedExchange::Config::Config()
    : fillLatencyMs(engine::Config::getInt("exchange.fill_latency_ms", 10))
    , rejectionRate(engine::Config::getDouble("exchange.rejection_rate", 0.0))
    , partialFillRate(engine::Config::getDouble("exchange.partial_fill_rate", 0.0))
    , slippageBps(engine::Config::getDouble("exchange.slippage_bps", 5.0))
    , instantFills(engine::Config::getBool("exchange.instant_fills", false)) 
{}

SimulatedExchange::SimulatedExchange(const Config& config)
    : config_(config)
    , isRunning_(false)
    , orderSubId_(0)
    , randomEngine_(std::random_device{}())
{}

SimulatedExchange::~SimulatedExchange() {
    stop();
}

void SimulatedExchange::start() {
    if (isRunning_) return;

    isRunning_ = true;

    // Subscribe to ORDER events
    orderSubId_ = EventBus::getInstance().subscribe(
        EventType::ORDER,
        [this](const Event& event) { this->onOrderEvent(event); }
    );
}

void SimulatedExchange::stop() {
    if (!isRunning_) return;

    isRunning_ = false;
    EventBus::getInstance().unsubscribe(orderSubId_);
}

bool SimulatedExchange::isRunning() const {
    return isRunning_;
}

void SimulatedExchange::submitOrder(const std::string& orderId,
                const std::string& symbol,
                Side side,
                OrderType type,
                double price,
                int64_t quantity) {
    // Check for rejection
    if (shouldReject()) {
        OrderEvent rejectEvent(orderId, symbol, side, type,
                              OrderStatus::REJECTED, price, quantity);
        EventBus::getInstance().publish(rejectEvent);
        return;
    }

    // Accept the order
    OrderEvent newEvent(orderId, symbol, side, type,
                       OrderStatus::NEW, price, quantity);
    EventBus::getInstance().publish(newEvent);

    // Schedule fill
    if (config_.instantFills) {
        processFill(orderId, symbol, side, type, price, quantity);
    } else {
        std::thread([this, orderId, symbol, side, type, price, quantity]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.fillLatencyMs));
            if (isRunning_) {
                processFill(orderId, symbol, side, type, price, quantity);
            }
        }).detach();
    }
}

void SimulatedExchange::cancelOrder(const std::string& orderId) {
    // Find order and publish cancel events
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pendingOrders_.find(orderId);
    if (it != pendingOrders_.end()) {
        OrderEvent cancelEvent(it->second.orderId, it->second.symbol,
                              it->second.side, it->second.type,
                              OrderStatus::CANCELLED,
                              it->second.price, it->second.quantity);
        EventBus::getInstance().publish(cancelEvent);
        pendingOrders_.erase(it);
    }
}

void SimulatedExchange::setMarketPrice(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(mutex_);
    marketPrices_[symbol] = price;
}

const SimulatedExchange::Config& SimulatedExchange::getConfig() const {
    return config_;
}

void SimulatedExchange::setConfig(const Config& config) {
    config_ = config;
}

void SimulatedExchange::onOrderEvent(const Event& event) {
    const auto* orderEvent = dynamic_cast<const OrderEvent*>(&event);
    if (!orderEvent) return;

    // Only process PENDING_NEW orders (submitted by Portfolio/OrderManager)
    if (orderEvent->getStatus() == OrderStatus::PENDING_NEW) {
        submitOrder(orderEvent->getOrderId(),
                   orderEvent->getSymbol(),
                   orderEvent->getSide(),
                   orderEvent->getOrderType(),
                   orderEvent->getPrice(),
                   orderEvent->getQuantity());
    }
    else if (orderEvent->getStatus() == OrderStatus::PENDING_CANCEL) {
        cancelOrder(orderEvent->getOrderId());
    }
}

void SimulatedExchange::processFill(const std::string& orderId,
                const std::string& symbol,
                Side side,
                OrderType type,
                double price,
                int64_t quantity) {
    // Calculate fill price (with slippage for market orders)
    double fillPrice = price;
    if (type == OrderType::MARKET) {
        fillPrice = applySlippage(symbol, side, price);
    }

    // Determine fill quantity (check for partial fills)
    int64_t fillQty = quantity;
    if (shouldPartialFill()) {
        // Fill 50-90% of the order
        std::uniform_real_distribution<> dist(0.5, 0.9);
        fillQty = static_cast<int64_t>(quantity * dist(randomEngine_));
        fillQty = std::max(fillQty, int64_t(1)); // At least 1 share
    }

    // Publish fill event
    FillEvent fillEvent(orderId, symbol, side, fillPrice, fillQty);
    EventBus::getInstance().publish(fillEvent);

    // If partial fill, update order status
    if (fillQty < quantity) {
        OrderEvent partialEvent(orderId, symbol, side, type,
                               OrderStatus::PARTIALLY_FILLED,
                               price, quantity, fillQty);
        EventBus::getInstance().publish(partialEvent);

        // Schedule remaining fill
        int64_t remaining = quantity - fillQty;
        if (!config_.instantFills) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.fillLatencyMs));
        }
        if (isRunning_) {
            FillEvent remainingFill(orderId, symbol, side, fillPrice, remaining);
            EventBus::getInstance().publish(remainingFill);
        }
    }

    // Publish FILLED status
    OrderEvent filledEvent(orderId, symbol, side, type,
                          OrderStatus::FILLED, price, quantity, quantity);
    EventBus::getInstance().publish(filledEvent);
}

bool SimulatedExchange::shouldReject() {
    if (config_.rejectionRate <= 0.0) return false;
    std::uniform_real_distribution<> dist(0.0, 1.0);
    return dist(randomEngine_) < config_.rejectionRate;
}

bool SimulatedExchange::shouldPartialFill() {
    if (config_.partialFillRate <= 0.0) return false;
    std::uniform_real_distribution<> dist(0.0, 1.0);
    return dist(randomEngine_) < config_.partialFillRate;
}

double SimulatedExchange::applySlippage(const std::string& symbol, Side side, double price) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Use market price if available, otherwise use order price
    double basePrice = price;
    auto it = marketPrices_.find(symbol);
    if (it != marketPrices_.end()) {
        basePrice = it->second;
    }

    // Apply slippage (negative for buy, positive for sell)
    double slippageFactor = config_.slippageBps / 10000.0;
    if (side == Side::BUY) {
        return basePrice * (1.0 + slippageFactor);  // Pay more
    } else {
        return basePrice * (1.0 - slippageFactor);  // Receive less
    }
}

} // namespace engine
