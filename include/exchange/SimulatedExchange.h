#pragma once

#include "config/Config.h"
#include "event/EventBus.h"
#include "event/OrderEvent.h"
#include "exchange/ExchangeConnector.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>

namespace engine {

/**
 * SimulatedExchange: Exchange simulator for testing and backtesting.
 * 
 * Simulates realistic exchange behavior including:
 * - Order acceptance/rejection
 * - Fill latency
 * - Partial fills
 * - Slippage for market orders
 * - Order book price levels
 */
class SimulatedExchange : public ExchangeConnector {
public:
    /**
     * Configuration for simulation behavior
     */
    struct Config {
        int fillLatencyMs;
        double rejectionRate;
        double partialFillRate;
        double slippageBps;
        bool instantFills;

        Config()
            : fillLatencyMs(engine::Config::getInt("exchange.fill_latency_ms", 10))
            , rejectionRate(engine::Config::getDouble("exchange.rejection_rate", 0.0))
            , partialFillRate(engine::Config::getDouble("exchange.partial_fill_rate", 0.0))
            , slippageBps(engine::Config::getDouble("exchange.slippage_bps", 5.0))
            , instantFills(engine::Config::getBool("exchange.instant_fills", false)) {}
    };

    explicit SimulatedExchange(const Config& config = Config())
        : config_(config)
        , isRunning_(false)
        , randomEngine_(std::random_device{}())
    {}

    ~SimulatedExchange() {
        stop();
    }

    void start() override {
        if (isRunning_) return;

        isRunning_ = true;

        // Subscribe to ORDER events
        orderSubId_ = EventBus::getInstance().subscribe(
            EventType::ORDER,
            [this](const Event& event) { this->onOrderEvent(event); }
        );
    }

    void stop() override {
        if (!isRunning_) return;

        isRunning_ = false;
        EventBus::getInstance().unsubscribe(orderSubId_);
    }

    bool isRunning() const override {
        return isRunning_;
    }

    void submitOrder(const std::string& orderId,
                    const std::string& symbol,
                    Side side,
                    OrderType type,
                    double price,
                    int64_t quantity) override {
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

    void cancelOrder(const std::string& orderId) override {
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

    /**
     * Set current market price for a symbol (for slippage calculation)
     */
    void setMarketPrice(const std::string& symbol, double price) {
        std::lock_guard<std::mutex> lock(mutex_);
        marketPrices_[symbol] = price;
    }

    /**
     * Get configuration
     */
    const Config& getConfig() const {
        return config_;
    }

    /**
     * Update configuration
     */
    void setConfig(const Config& config) {
        config_ = config;
    }

private:
    struct PendingOrder {
        std::string orderId;
        std::string symbol;
        Side side;
        OrderType type;
        double price;
        int64_t quantity;
    };

    void onOrderEvent(const Event& event) {
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

    void processFill(const std::string& orderId,
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

    bool shouldReject() {
        if (config_.rejectionRate <= 0.0) return false;
        std::uniform_real_distribution<> dist(0.0, 1.0);
        return dist(randomEngine_) < config_.rejectionRate;
    }

    bool shouldPartialFill() {
        if (config_.partialFillRate <= 0.0) return false;
        std::uniform_real_distribution<> dist(0.0, 1.0);
        return dist(randomEngine_) < config_.partialFillRate;
    }

    double applySlippage(const std::string& symbol, Side side, double price) {
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

    Config config_;
    bool isRunning_;
    uint64_t orderSubId_;

    std::unordered_map<std::string, PendingOrder> pendingOrders_;
    std::unordered_map<std::string, double> marketPrices_;

    std::mt19937 randomEngine_;
    mutable std::mutex mutex_;
};

} // namespace engine
