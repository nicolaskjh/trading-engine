#include "event/EventBus.h"
#include "event/OrderEvent.h"
#include "logger/Logger.h"
#include "order/OrderLogger.h"
#include "order/OrderManager.h"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

using namespace engine;

void testOrderSubmission() {
    Logger::info(LogComponent::TEST, "=== Testing Order Submission ===");
    
    OrderManager orderManager;
    OrderLogger orderLogger;
    
    orderManager.submitOrder("ORD001", "AAPL", Side::BUY, OrderType::LIMIT, 150.25, 100);
    EventBus::getInstance().processQueue(10);
    
    auto order = orderManager.getOrder("ORD001");
    assert(order != nullptr);
    assert(order->getOrderId() == "ORD001");
    assert(order->getSymbol() == "AAPL");
    assert(order->getSide() == Side::BUY);
    assert(order->getQuantity() == 100);
    
    Logger::info(LogComponent::TEST, "✓ Order submission working");
}

void testOrderLifecycle() {
    Logger::info(LogComponent::TEST, "=== Testing Order Lifecycle ===");
    
    OrderManager orderManager;
    OrderLogger orderLogger;
    
    // Submit order
    orderManager.submitOrder("ORD002", "TSLA", Side::BUY, OrderType::LIMIT, 250.50, 200);
    
    // Exchange accepts
    OrderEvent accepted("ORD002", "TSLA", Side::BUY, OrderType::LIMIT,
                       OrderStatus::NEW, 250.50, 200);
    EventBus::getInstance().publish(accepted);
    EventBus::getInstance().processQueue(10);
    
    // Partial fill
    FillEvent fill1("ORD002", "TSLA", Side::BUY, 250.50, 100);
    EventBus::getInstance().publish(fill1);
    
    OrderEvent partialFilled("ORD002", "TSLA", Side::BUY, OrderType::LIMIT,
                             OrderStatus::PARTIALLY_FILLED, 250.50, 200, 100);
    EventBus::getInstance().publish(partialFilled);
    EventBus::getInstance().processQueue(10);
    
    auto order = orderManager.getOrder("ORD002");
    assert(order->getFilledQuantity() == 100);
    assert(!order->isFilled());
    
    // Complete fill
    FillEvent fill2("ORD002", "TSLA", Side::BUY, 250.55, 100);
    EventBus::getInstance().publish(fill2);
    
    OrderEvent filled("ORD002", "TSLA", Side::BUY, OrderType::LIMIT,
                     OrderStatus::FILLED, 250.50, 200, 200);
    EventBus::getInstance().publish(filled);
    EventBus::getInstance().processQueue(10);
    
    order = orderManager.getOrder("ORD002");
    assert(order->isFilled());
    assert(order->getFilledQuantity() == 200);
    
    Logger::info(LogComponent::TEST, "✓ Order lifecycle working");
}

void testPositionTracking() {
    Logger::info(LogComponent::TEST, "=== Testing Position Tracking ===");
    
    OrderManager orderManager;
    OrderLogger orderLogger;
    
    // Open position
    orderManager.submitOrder("ORD003", "GOOGL", Side::BUY, OrderType::LIMIT, 3000.00, 10);
    
    FillEvent fill1("ORD003", "GOOGL", Side::BUY, 3000.00, 5);
    EventBus::getInstance().publish(fill1);
    
    FillEvent fill2("ORD003", "GOOGL", Side::BUY, 3000.50, 5);
    EventBus::getInstance().publish(fill2);
    EventBus::getInstance().processQueue(10);
    
    auto position = orderManager.getPosition("GOOGL");
    assert(position != nullptr);
    assert(position->getQuantity() == 10);
    
    // Check average entry price
    double expectedAvg = (3000.00 * 5 + 3000.50 * 5) / 10;
    assert(std::abs(position->getAveragePrice() - expectedAvg) < 0.01);
    
    Logger::info(LogComponent::TEST, "✓ Position tracking working");
}

void testPnLCalculation() {
    Logger::info(LogComponent::TEST, "=== Testing P&L Calculation ===");
    
    OrderManager orderManager;
    OrderLogger orderLogger;
    
    // Buy 100 shares
    orderManager.submitOrder("ORD004", "AAPL", Side::BUY, OrderType::LIMIT, 150.00, 50);
    FillEvent buyFill1("ORD004", "AAPL", Side::BUY, 150.25, 50);
    EventBus::getInstance().publish(buyFill1);
    
    orderManager.submitOrder("ORD005", "AAPL", Side::BUY, OrderType::LIMIT, 150.00, 50);
    FillEvent buyFill2("ORD005", "AAPL", Side::BUY, 150.26, 50);
    EventBus::getInstance().publish(buyFill2);
    EventBus::getInstance().processQueue(10);
    
    // Sell 100 shares at higher price
    orderManager.submitOrder("ORD006", "AAPL", Side::SELL, OrderType::LIMIT, 150.35, 100);
    FillEvent sellFill("ORD006", "AAPL", Side::SELL, 150.35, 100);
    EventBus::getInstance().publish(sellFill);
    EventBus::getInstance().processQueue(10);
    
    auto position = orderManager.getPosition("AAPL");
    assert(position != nullptr);
    assert(position->getQuantity() == 0);
    
    // P&L should be positive
    double pnl = position->getRealizedPnL();
    assert(pnl > 0);
    
    std::ostringstream oss;
    oss << "Realized P&L: $" << std::fixed << std::setprecision(2) << pnl;
    Logger::info(LogComponent::TEST, oss.str());
    
    // Expected: (150.35 - 150.255) * 100 = $9.50
    assert(std::abs(pnl - 9.50) < 0.01);
    
    Logger::info(LogComponent::TEST, "✓ P&L calculation correct ($9.50)");
}

void testMultiplePositions() {
    Logger::info(LogComponent::TEST, "=== Testing Multiple Positions ===");
    
    OrderManager orderManager;
    
    // Create positions in multiple symbols
    FillEvent fill1("ORD007", "AAPL", Side::BUY, 150.00, 100);
    FillEvent fill2("ORD008", "TSLA", Side::BUY, 250.00, 50);
    FillEvent fill3("ORD009", "GOOGL", Side::BUY, 3000.00, 10);
    
    EventBus::getInstance().publish(fill1);
    EventBus::getInstance().publish(fill2);
    EventBus::getInstance().publish(fill3);
    EventBus::getInstance().processQueue(10);
    
    assert(orderManager.getPosition("AAPL") != nullptr);
    assert(orderManager.getPosition("TSLA") != nullptr);
    assert(orderManager.getPosition("GOOGL") != nullptr);
    
    Logger::info(LogComponent::TEST, "✓ Multiple positions tracked");
}

#ifndef TEST_ALL
int main() {
    Logger::init(LogLevel::INFO);
    
    try {
        Logger::info(LogComponent::TEST, "Starting OrderManager Tests...\n");
        
        testOrderSubmission();
        testOrderLifecycle();
        testPositionTracking();
        testPnLCalculation();
        testMultiplePositions();
        
        Logger::info(LogComponent::TEST, "\n✓ All OrderManager tests passed!");
        
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Test failed: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
#endif
