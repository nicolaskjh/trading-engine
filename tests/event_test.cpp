#include "data/MarketDataHandler.h"
#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "event/OrderEvent.h"
#include "event/TimerEvent.h"
#include "logger/Logger.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace engine;

void testEventPublishSubscribe() {
    Logger::info(LogComponent::TEST, "=== Testing Event Publish/Subscribe ===");
    
    MarketDataHandler mdHandler;
    
    // Publish market data events
    QuoteEvent quote1("AAPL", 150.25, 150.27, 100, 200);
    EventBus::getInstance().publish(quote1);
    
    QuoteEvent quote2("TSLA", 250.50, 250.55, 300, 150);
    EventBus::getInstance().publish(quote2);
    
    TradeEvent trade1("AAPL", 150.26, 500);
    EventBus::getInstance().publish(trade1);
    
    Logger::info(LogComponent::TEST, "✓ Published 3 market data events");
}

void testSystemEvents() {
    Logger::info(LogComponent::TEST, "=== Testing System Events ===");
    
    int eventCount = 0;
    
    size_t subId = EventBus::getInstance().subscribe(
        EventType::SYSTEM,
        [&eventCount](const Event& event) {
            const auto* sysEvent = dynamic_cast<const SystemEvent*>(&event);
            if (sysEvent) {
                eventCount++;
                Logger::info(LogComponent::SYSTEM, sysEvent->getMessage());
            }
        }
    );
    
    SystemEvent startup(SystemEventType::TRADING_START, "Trading session started");
    EventBus::getInstance().publish(startup);
    EventBus::getInstance().processQueue(10);
    
    assert(eventCount == 1);
    EventBus::getInstance().unsubscribe(subId);
    
    Logger::info(LogComponent::TEST, "✓ System events working");
}

void testTimerEvents() {
    Logger::info(LogComponent::TEST, "=== Testing Timer Events ===");
    
    bool callbackExecuted = false;
    
    size_t subId = EventBus::getInstance().subscribe(
        EventType::TIMER,
        [&callbackExecuted](const Event& event) {
            const auto* timer = dynamic_cast<const TimerEvent*>(&event);
            if (timer) {
                Logger::info(LogComponent::TIMER, "'" + timer->getName() + "' fired");
                if (timer->hasCallback()) {
                    timer->execute();
                }
            }
        }
    );
    
    TimerEvent heartbeat("heartbeat", [&callbackExecuted]() {
        Logger::debug(LogComponent::TIMER, "Heartbeat callback executed");
        callbackExecuted = true;
    });
    
    EventBus::getInstance().publish(heartbeat);
    EventBus::getInstance().processQueue(10);
    
    assert(callbackExecuted);
    EventBus::getInstance().unsubscribe(subId);
    
    Logger::info(LogComponent::TEST, "✓ Timer events and callbacks working");
}

void testEventCount() {
    Logger::info(LogComponent::TEST, "=== Testing Event Count ===");
    
    size_t initialCount = EventBus::getInstance().getEventCount();
    
    QuoteEvent quote("TEST", 100.0, 100.5, 10, 20);
    EventBus::getInstance().publish(quote);
    EventBus::getInstance().processQueue(10);
    
    size_t newCount = EventBus::getInstance().getEventCount();
    assert(newCount > initialCount);
    
    std::ostringstream oss;
    oss << "Total events processed: " << newCount;
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Event counting working");
}

#ifndef TEST_ALL
int main() {
    Logger::init(LogLevel::INFO);
    
    try {
        Logger::info(LogComponent::TEST, "Starting Event System Tests...\n");
        
        testEventPublishSubscribe();
        testSystemEvents();
        testTimerEvents();
        testEventCount();
        
        Logger::info(LogComponent::TEST, "\n✓ All Event System tests passed!");
        
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Test failed: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
#endif
