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
    StrategyManager();
    ~StrategyManager();

    // Prevent copying
    StrategyManager(const StrategyManager&) = delete;
    StrategyManager& operator=(const StrategyManager&) = delete;

    /**
     * Add a strategy to the manager
     */
    void addStrategy(std::shared_ptr<Strategy> strategy);

    /**
     * Remove a strategy by name
     */
    bool removeStrategy(const std::string& name);

    /**
     * Get strategy by name
     */
    std::shared_ptr<Strategy> getStrategy(const std::string& name) const;

    /**
     * Get all strategies
     */
    std::vector<std::shared_ptr<Strategy>> getAllStrategies() const;

    /**
     * Get count of registered strategies
     */
    size_t getStrategyCount() const;

    /**
     * Start all strategies
     */
    void startAll();

    /**
     * Stop all strategies
     */
    void stopAll();

    /**
     * Start a specific strategy
     */
    bool startStrategy(const std::string& name);

    /**
     * Stop a specific strategy
     */
    bool stopStrategy(const std::string& name);

private:
    /**
     * Handle market data events and route to strategies
     */
    void onMarketDataEvent(const Event& event);

    /**
     * Handle order events and route to strategies
     */
    void onOrderEvent(const Event& event);

    /**
     * Handle fill events and route to strategies
     */
    void onFillEvent(const Event& event);

    /**
     * Get strategy without locking (caller must hold lock)
     */
    std::shared_ptr<Strategy> getStrategyUnlocked(const std::string& name) const;

    std::vector<std::shared_ptr<Strategy>> strategies_;
    uint64_t marketDataSubId_;
    uint64_t orderSubId_;
    uint64_t fillSubId_;
    mutable std::recursive_mutex mutex_;
};

} // namespace engine
