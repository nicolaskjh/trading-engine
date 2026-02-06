#pragma once

#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "event/OrderEvent.h"
#include "strategy/Strategy.h"

#include <memory>
#include <mutex>
#include <vector>

namespace engine {

/**
 * StrategyManager: Coordinates multiple trading strategies.
 * 
 * Responsibilities:
 * - Register and manage multiple strategies
 * - Route events to all registered strategies
 * - Control strategy lifecycle (start/stop)
 * - Provide centralized strategy monitoring
 */
class StrategyManager {
public:
    StrategyManager() {
        // Subscribe to market data events (MARKET_DATA type covers both quotes and trades)
        marketDataSubId_ = EventBus::getInstance().subscribe(
            EventType::MARKET_DATA,
            [this](const Event& event) { this->onMarketDataEvent(event); }
        );

        // Subscribe to order events
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

    ~StrategyManager() {
        EventBus::getInstance().unsubscribe(marketDataSubId_);
        EventBus::getInstance().unsubscribe(orderSubId_);
        EventBus::getInstance().unsubscribe(fillSubId_);
    }

    // Prevent copying
    StrategyManager(const StrategyManager&) = delete;
    StrategyManager& operator=(const StrategyManager&) = delete;

    /**
     * Add a strategy to the manager
     */
    void addStrategy(std::shared_ptr<Strategy> strategy) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        strategies_.push_back(strategy);
    }

    /**
     * Remove a strategy by name
     */
    bool removeStrategy(const std::string& name) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto it = std::find_if(strategies_.begin(), strategies_.end(),
            [&name](const std::shared_ptr<Strategy>& s) {
                return s->getName() == name;
            });
        
        if (it != strategies_.end()) {
            (*it)->stop();
            strategies_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * Get strategy by name
     */
    std::shared_ptr<Strategy> getStrategy(const std::string& name) const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto it = std::find_if(strategies_.begin(), strategies_.end(),
            [&name](const std::shared_ptr<Strategy>& s) {
                return s->getName() == name;
            });
        
        return (it != strategies_.end()) ? *it : nullptr;
    }

    /**
     * Get all strategies
     */
    std::vector<std::shared_ptr<Strategy>> getAllStrategies() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return strategies_;
    }

    /**
     * Get count of registered strategies
     */
    size_t getStrategyCount() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return strategies_.size();
    }

    /**
     * Start all strategies
     */
    void startAll() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& strategy : strategies_) {
            strategy->start();
        }
    }

    /**
     * Stop all strategies
     */
    void stopAll() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& strategy : strategies_) {
            strategy->stop();
        }
    }

    /**
     * Start a specific strategy
     */
    bool startStrategy(const std::string& name) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto strategy = getStrategyUnlocked(name);
        if (strategy) {
            strategy->start();
            return true;
        }
        return false;
    }

    /**
     * Stop a specific strategy
     */
    bool stopStrategy(const std::string& name) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto strategy = getStrategyUnlocked(name);
        if (strategy) {
            strategy->stop();
            return true;
        }
        return false;
    }

private:
    /**
     * Handle market data events and route to strategies
     */
    void onMarketDataEvent(const Event& event) {
        // Try to cast to TradeEvent first
        const auto* tradeEvent = dynamic_cast<const TradeEvent*>(&event);
        if (tradeEvent) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            for (auto& strategy : strategies_) {
                strategy->handleTradeEvent(*tradeEvent);
            }
            return;
        }

        // Try to cast to QuoteEvent
        const auto* quoteEvent = dynamic_cast<const QuoteEvent*>(&event);
        if (quoteEvent) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            for (auto& strategy : strategies_) {
                strategy->handleQuoteEvent(*quoteEvent);
            }
            return;
        }
    }

    /**
     * Handle order events and route to strategies
     */
    void onOrderEvent(const Event& event) {
        const auto* orderEvent = dynamic_cast<const OrderEvent*>(&event);
        if (!orderEvent) return;

        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& strategy : strategies_) {
            strategy->handleOrderEvent(*orderEvent);
        }
    }

    /**
     * Handle fill events and route to strategies
     */
    void onFillEvent(const Event& event) {
        const auto* fillEvent = dynamic_cast<const FillEvent*>(&event);
        if (!fillEvent) return;

        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& strategy : strategies_) {
            strategy->handleFillEvent(*fillEvent);
        }
    }

    /**
     * Get strategy without locking (caller must hold lock)
     */
    std::shared_ptr<Strategy> getStrategyUnlocked(const std::string& name) const {
        auto it = std::find_if(strategies_.begin(), strategies_.end(),
            [&name](const std::shared_ptr<Strategy>& s) {
                return s->getName() == name;
            });
        
        return (it != strategies_.end()) ? *it : nullptr;
    }

    std::vector<std::shared_ptr<Strategy>> strategies_;
    uint64_t marketDataSubId_;
    uint64_t orderSubId_;
    uint64_t fillSubId_;
    mutable std::recursive_mutex mutex_;
};

} // namespace engine
