#include "strategy/StrategyManager.h"

namespace engine {

StrategyManager::StrategyManager() {
    // Subscribe to market data events
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

StrategyManager::~StrategyManager() {
    EventBus::getInstance().unsubscribe(marketDataSubId_);
    EventBus::getInstance().unsubscribe(orderSubId_);
    EventBus::getInstance().unsubscribe(fillSubId_);
}

void StrategyManager::addStrategy(std::shared_ptr<Strategy> strategy) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    strategies_.push_back(strategy);
}

bool StrategyManager::removeStrategy(const std::string& name) {
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

std::shared_ptr<Strategy> StrategyManager::getStrategy(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = std::find_if(strategies_.begin(), strategies_.end(),
        [&name](const std::shared_ptr<Strategy>& s) {
            return s->getName() == name;
        });
    
    return (it != strategies_.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<Strategy>> StrategyManager::getAllStrategies() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return strategies_;
}

size_t StrategyManager::getStrategyCount() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return strategies_.size();
}

void StrategyManager::startAll() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& strategy : strategies_) {
        strategy->start();
    }
}

void StrategyManager::stopAll() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& strategy : strategies_) {
        strategy->stop();
    }
}

bool StrategyManager::startStrategy(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto strategy = getStrategyUnlocked(name);
    if (strategy) {
        strategy->start();
        return true;
    }
    return false;
}

bool StrategyManager::stopStrategy(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto strategy = getStrategyUnlocked(name);
    if (strategy) {
        strategy->stop();
        return true;
    }
    return false;
}

void StrategyManager::onMarketDataEvent(const Event& event) {
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

void StrategyManager::onOrderEvent(const Event& event) {
    const auto* orderEvent = dynamic_cast<const OrderEvent*>(&event);
    if (!orderEvent) return;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& strategy : strategies_) {
        strategy->handleOrderEvent(*orderEvent);
    }
}

void StrategyManager::onFillEvent(const Event& event) {
    const auto* fillEvent = dynamic_cast<const FillEvent*>(&event);
    if (!fillEvent) return;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& strategy : strategies_) {
        strategy->handleFillEvent(*fillEvent);
    }
}

std::shared_ptr<Strategy> StrategyManager::getStrategyUnlocked(const std::string& name) const {
    auto it = std::find_if(strategies_.begin(), strategies_.end(),
        [&name](const std::shared_ptr<Strategy>& s) {
            return s->getName() == name;
        });
    
    return (it != strategies_.end()) ? *it : nullptr;
}

} // namespace engine
