#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "event/OrderEvent.h"
#include "event/TimerEvent.h"
#include "order/OrderManager.h"
#include "market_data/MarketDataHandler.h"
#include "order/OrderLogger.h"
#include "logger/Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

using namespace engine;

/**
 * Test: OrderManager functionality with order lifecycle and position tracking
 */
void testOrderManager() {
    Logger::info(LogComponent::TEST, "=== OrderManager Test ===");
    
    OrderManager orderManager;
    OrderLogger orderLogger;
    
    Logger::info(LogComponent::TEST, "--- Test 1: Submit and Fill Order ---");
    
    // Submit BUY order
    Logger::info(LogComponent::TEST, "Submitting BUY order for 100 AAPL @ $150.25");
    orderManager.submitOrder("ORD001", "AAPL", Side::BUY, OrderType::LIMIT, 150.25, 100);
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    
    // Simulate exchange accepting order
    Logger::info(LogComponent::TEST, "Exchange accepted order");
    OrderEvent order2("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::NEW, 150.25, 100);
    EventBus::getInstance().publish(order2);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    // Simulate partial fill
    Logger::info(LogComponent::TEST, "Partial fill: 50 shares @ $150.25");
    FillEvent fill1("ORD001", "AAPL", Side::BUY, 150.25, 50);
    EventBus::getInstance().publish(fill1);
    
    OrderEvent order3("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::PARTIALLY_FILLED, 150.25, 100, 50);
    EventBus::getInstance().publish(order3);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    // Complete fill at different price
    Logger::info(LogComponent::TEST, "Complete fill: 50 shares @ $150.26");
    FillEvent fill2("ORD001", "AAPL", Side::BUY, 150.26, 50);
    EventBus::getInstance().publish(fill2);
    
    OrderEvent order4("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::FILLED, 150.25, 100, 100);
    EventBus::getInstance().publish(order4);
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    
    // Verify order state
    Logger::info(LogComponent::TEST, "--- Test Results ---");
    auto order = orderManager.getOrder("ORD001");
    if (order) {
        std::ostringstream oss;
        oss << "Order Status: " << (order->isFilled() ? "FILLED" : "ACTIVE")
            << " | Filled: " << order->getFilledQuantity() << "/" << order->getQuantity()
            << " | Avg Fill Price: $" << std::fixed << std::setprecision(4)
            << order->getAverageFillPrice();
        Logger::info(LogComponent::TEST, oss.str());
    }
    
    // Verify position
    auto position = orderManager.getPosition("AAPL");
    if (position) {
        std::ostringstream oss;
        oss << "Position: " << position->getQuantity() << " shares"
            << " | Avg Entry: $" << std::fixed << std::setprecision(4)
            << position->getAveragePrice()
            << " | Realized P&L: $" << std::setprecision(2)
            << position->getRealizedPnL();
        Logger::info(LogComponent::TEST, oss.str());
    }
    
    Logger::info(LogComponent::TEST, "--- Test 2: Close Position ---");
    
    // Submit SELL order to close
    Logger::info(LogComponent::TEST, "Submitting SELL order to close position: 100 @ $150.35");
    orderManager.submitOrder("ORD002", "AAPL", Side::SELL, OrderType::LIMIT, 150.35, 100);
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    
    OrderEvent order5("ORD002", "AAPL", Side::SELL, OrderType::LIMIT,
                      OrderStatus::NEW, 150.35, 100);
    EventBus::getInstance().publish(order5);
    
    Logger::info(LogComponent::TEST, "Complete fill: 100 shares @ $150.35");
    FillEvent fill3("ORD002", "AAPL", Side::SELL, 150.35, 100);
    EventBus::getInstance().publish(fill3);
    
    OrderEvent order6("ORD002", "AAPL", Side::SELL, OrderType::LIMIT,
                      OrderStatus::FILLED, 150.35, 100, 100);
    EventBus::getInstance().publish(order6);
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    
    // Final verification
    Logger::info(LogComponent::TEST, "--- Final Results ---");
    position = orderManager.getPosition("AAPL");
    if (position) {
        std::ostringstream oss;
        oss << "Final Position: " << position->getQuantity() << " shares"
            << " | Realized P&L: $" << std::setprecision(2)
            << position->getRealizedPnL();
        Logger::info(LogComponent::TEST, oss.str());
    }
    
    std::ostringstream oss;
    oss << "Portfolio Total Realized P&L: $" << orderManager.getTotalRealizedPnL();
    Logger::info(LogComponent::TEST, oss.str());
    
    oss.str("");
    oss << "Active Orders: " << orderManager.getActiveOrderCount();
    Logger::info(LogComponent::TEST, oss.str());
}

/**
 * Test: Event system functionality
 */
void testEventSystem() {
    Logger::info(LogComponent::TEST, "=== Event System Test ===");
    
    MarketDataHandler mdHandler;
    
    Logger::info(LogComponent::TEST, "Publishing Market Data Events");
    
    QuoteEvent quote1("AAPL", 150.25, 150.27, 100, 200);
    EventBus::getInstance().publish(quote1);
    
    QuoteEvent quote2("TSLA", 250.50, 250.55, 300, 150);
    EventBus::getInstance().publish(quote2);
    
    TradeEvent trade1("AAPL", 150.26, 500);
    EventBus::getInstance().publish(trade1);
    
    Logger::info(LogComponent::TEST, "Published 3 market data events");
    
    // System events
    Logger::info(LogComponent::TEST, "Testing System Events");
    EventBus::getInstance().subscribe(
        EventType::SYSTEM,
        [](const Event& event) {
            const auto* sysEvent = dynamic_cast<const SystemEvent*>(&event);
            if (sysEvent) {
                Logger::info(LogComponent::SYSTEM, sysEvent->getMessage());
            }
        }
    );
    
    SystemEvent startup(SystemEventType::TRADING_START, "Trading session started");
    EventBus::getInstance().publish(startup);
    
    // Timer events
    Logger::info(LogComponent::TEST, "Testing Timer Events");
    EventBus::getInstance().subscribe(
        EventType::TIMER,
        [](const Event& event) {
            const auto* timer = dynamic_cast<const TimerEvent*>(&event);
            if (timer) {
                Logger::info(LogComponent::TIMER, "'" + timer->getName() + "' fired");
                if (timer->hasCallback()) {
                    timer->execute();
                }
            }
        }
    );
    
    TimerEvent heartbeat("heartbeat", []() {
        Logger::debug(LogComponent::TIMER, "Heartbeat callback executed");
    });
    EventBus::getInstance().publish(heartbeat);
    
    std::ostringstream oss;
    oss << "Total events processed: " << EventBus::getInstance().getEventCount();
    Logger::info(LogComponent::TEST, oss.str());
    
    oss.str("");
    oss << "Queue size: " << EventBus::getInstance().getQueueSize();
    Logger::info(LogComponent::TEST, oss.str());
}

/**
 * Main test runner
 */
int main() {
    // Initialize logger with DEBUG level for tests
    Logger::init(LogLevel::DEBUG);
    
    std::cout << "========================================\n";
    std::cout << "   Trading Engine Component Tests\n";
    std::cout << "========================================\n\n";
    
    try {
        testEventSystem();
        std::cout << "\n\n";
        testOrderManager();
        
        std::cout << "\n========================================\n";
        std::cout << "   All Tests Completed Successfully\n";
        std::cout << "========================================\n";
        
        Logger::shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Test failed with exception: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
}
