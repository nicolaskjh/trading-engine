#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "risk/Portfolio.h"
#include "strategy/SMAStrategy.h"
#include "strategy/StrategyManager.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using namespace engine;

void test_strategy_lifecycle() {
    std::cout << "Testing strategy lifecycle..." << std::endl;
    
    auto portfolio = std::make_shared<Portfolio>(100000.0);
    auto strategy = std::make_shared<SMAStrategy>("test_sma", portfolio, "AAPL", 10, 30, 100);
    
    assert(!strategy->isRunning());
    
    strategy->start();
    assert(strategy->isRunning());
    assert(strategy->getName() == "test_sma");
    
    strategy->stop();
    assert(!strategy->isRunning());
    
    std::cout << "  ✓ Strategy lifecycle passed" << std::endl;
}

void test_strategy_manager() {
    std::cout << "Testing strategy manager..." << std::endl;
    
    StrategyManager manager;
    
    auto portfolio1 = std::make_shared<Portfolio>(100000.0);
    auto portfolio2 = std::make_shared<Portfolio>(200000.0);
    
    auto strategy1 = std::make_shared<SMAStrategy>("sma1", portfolio1, "AAPL", 10, 30, 100);
    auto strategy2 = std::make_shared<SMAStrategy>("sma2", portfolio2, "GOOGL", 5, 20, 50);
    
    manager.addStrategy(strategy1);
    manager.addStrategy(strategy2);
    
    assert(manager.getStrategyCount() == 2);
    
    auto retrieved = manager.getStrategy("sma1");
    assert(retrieved != nullptr);
    assert(retrieved->getName() == "sma1");
    
    manager.startAll();
    assert(strategy1->isRunning());
    assert(strategy2->isRunning());
    
    manager.stopAll();
    assert(!strategy1->isRunning());
    assert(!strategy2->isRunning());
    
    bool removed = manager.removeStrategy("sma1");
    assert(removed);
    assert(manager.getStrategyCount() == 1);
    
    std::cout << "  ✓ Strategy manager passed" << std::endl;
}

void test_sma_calculation() {
    std::cout << "Testing SMA calculation..." << std::endl;
    
    auto portfolio = std::make_shared<Portfolio>(100000.0);
    auto strategy = std::make_shared<SMAStrategy>("test_sma", portfolio, "AAPL", 3, 5, 100);
    
    strategy->start();
    
    // Send price updates
    double prices[] = {100.0, 102.0, 101.0, 103.0, 104.0, 105.0};
    
    for (double price : prices) {
        TradeEvent event("AAPL", price, 100);
        strategy->handleTradeEvent(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // After 6 prices, should have valid SMAs
    assert(strategy->getPriceCount() == 6);
    
    // Fast SMA (3): average of last 3 prices = (103 + 104 + 105) / 3 = 104.0
    double fastSMA = strategy->getFastSMA();
    assert(std::abs(fastSMA - 104.0) < 0.01);
    
    // Slow SMA (5): average of last 5 prices = (102 + 101 + 103 + 104 + 105) / 5 = 103.0
    double slowSMA = strategy->getSlowSMA();
    assert(std::abs(slowSMA - 103.0) < 0.01);
    
    std::cout << "  ✓ SMA calculation passed (Fast=" << fastSMA << ", Slow=" << slowSMA << ")" << std::endl;
}

void test_sma_crossover_buy_signal() {
    std::cout << "Testing SMA crossover buy signal..." << std::endl;
    
    StrategyManager manager;
    auto portfolio = std::make_shared<Portfolio>(1000000.0);
    auto strategy = std::make_shared<SMAStrategy>("buy_test", portfolio, "AAPL", 2, 3, 100);
    
    manager.addStrategy(strategy);
    strategy->start();
    
    // Create trend where fast will cross above slow
    // Prices: 100, 99, 98 (downtrend) then 100, 102 (uptrend)
    double prices[] = {100.0, 99.0, 98.0, 100.0, 102.0};
    
    for (double price : prices) {
        TradeEvent event("AAPL", price, 100);
        EventBus::getInstance().publish(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Simulate fills for any pending orders (simple exchange simulator)
        auto orders = portfolio->getOrderManager()->getActiveOrders();
        for (const auto& order : orders) {
            if (order->getSymbol() == "AAPL") {
                FillEvent fill(order->getOrderId(), order->getSymbol(), order->getSide(),
                             order->getQuantity() - order->getFilledQuantity(), price);
                EventBus::getInstance().publish(fill);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Should have bought (golden cross)
    auto position = portfolio->getOrderManager()->getPosition("AAPL");
    assert(position != nullptr);
    assert(position->getQuantity() > 0);
    
    std::cout << "  ✓ SMA crossover buy signal passed (Position: " << position->getQuantity() << ")" << std::endl;
    
    // Stop strategy and give time for any pending events to finish
    manager.stopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void test_sma_crossover_sell_signal() {
    std::cout << "Testing SMA crossover sell signal..." << std::endl;
    
    StrategyManager manager;
    auto portfolio = std::make_shared<Portfolio>(1000000.0);
    auto strategy = std::make_shared<SMAStrategy>("sell_test", portfolio, "GOOGL", 2, 3, 50);
    
    manager.addStrategy(strategy);
    strategy->start();
    
    // Create uptrend first (to go long), then downtrend (to sell)
    // Uptrend: 100, 102, 104 -> then downtrend: 102, 100
    double prices[] = {100.0, 102.0, 104.0, 102.0, 100.0};
    
    for (double price : prices) {
        TradeEvent event("GOOGL", price, 100);
        EventBus::getInstance().publish(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Simulate fills for any pending orders
        auto orders = portfolio->getOrderManager()->getActiveOrders();
        for (const auto& order : orders) {
            if (order->getSymbol() == "GOOGL") {
                FillEvent fill(order->getOrderId(), order->getSymbol(), order->getSide(),
                             order->getQuantity() - order->getFilledQuantity(), price);
                EventBus::getInstance().publish(fill);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Should have sold (death cross)
    auto position = portfolio->getOrderManager()->getPosition("GOOGL");
    assert(position != nullptr);
    assert(position->getQuantity() < 0);  // Short position
    
    std::cout << "  ✓ SMA crossover sell signal passed (Position: " << position->getQuantity() << ")" << std::endl;
    
    // Stop strategy and give time for any pending events to finish
    manager.stopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void test_strategy_manager_routing() {
    std::cout << "Testing strategy manager event routing..." << std::endl;
    
    StrategyManager manager;
    
    auto portfolio1 = std::make_shared<Portfolio>(1000000.0);
    auto portfolio2 = std::make_shared<Portfolio>(1000000.0);
    
    auto strategy1 = std::make_shared<SMAStrategy>("aapl_strat", portfolio1, "AAPL", 2, 3, 100);
    auto strategy2 = std::make_shared<SMAStrategy>("googl_strat", portfolio2, "GOOGL", 2, 3, 50);
    
    manager.addStrategy(strategy1);
    manager.addStrategy(strategy2);
    manager.startAll();
    
    // Send market data for AAPL - only strategy1 should react
    double aaplPrices[] = {100.0, 102.0, 104.0, 102.0, 100.0};
    for (double price : aaplPrices) {
        TradeEvent event("AAPL", price, 100);
        EventBus::getInstance().publish(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Simulate fills
        auto orders = portfolio1->getOrderManager()->getActiveOrders();
        for (const auto& order : orders) {
            FillEvent fill(order->getOrderId(), order->getSymbol(), order->getSide(),
                         order->getQuantity() - order->getFilledQuantity(), price);
            EventBus::getInstance().publish(fill);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Send market data for GOOGL - only strategy2 should react
    double googlPrices[] = {2800.0, 2820.0, 2840.0, 2820.0, 2800.0};
    for (double price : googlPrices) {
        TradeEvent event("GOOGL", price, 10);
        EventBus::getInstance().publish(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Simulate fills
        auto orders = portfolio2->getOrderManager()->getActiveOrders();
        for (const auto& order : orders) {
            FillEvent fill(order->getOrderId(), order->getSymbol(), order->getSide(),
                         order->getQuantity() - order->getFilledQuantity(), price);
            EventBus::getInstance().publish(fill);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Both strategies should have positions
    auto pos1 = portfolio1->getOrderManager()->getPosition("AAPL");
    auto pos2 = portfolio2->getOrderManager()->getPosition("GOOGL");
    
    assert(pos1 != nullptr);
    assert(pos2 != nullptr);
    assert(pos1->getQuantity() != 0);
    assert(pos2->getQuantity() != 0);
    
    manager.stopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    std::cout << "  ✓ Strategy manager routing passed" << std::endl;
}

void test_strategy_ignores_wrong_symbol() {
    std::cout << "Testing strategy ignores wrong symbol..." << std::endl;
    
    auto portfolio = std::make_shared<Portfolio>(1000000.0);
    auto strategy = std::make_shared<SMAStrategy>("aapl_only", portfolio, "AAPL", 2, 3, 100);
    
    strategy->start();
    
    // Send GOOGL data (should be ignored)
    for (int i = 0; i < 10; ++i) {
        TradeEvent event("GOOGL", 2800.0 + i, 10);
        strategy->handleTradeEvent(event);
    }
    
    // Should have no AAPL prices
    assert(strategy->getPriceCount() == 0);
    
    // Send AAPL data (should be processed)
    for (int i = 0; i < 5; ++i) {
        TradeEvent event("AAPL", 100.0 + i, 100);
        strategy->handleTradeEvent(event);
    }
    
    // Should have AAPL prices now
    assert(strategy->getPriceCount() == 5);
    
    std::cout << "  ✓ Strategy symbol filtering passed" << std::endl;
}

void test_strategy_waits_for_sufficient_data() {
    std::cout << "Testing strategy waits for sufficient data..." << std::endl;
    
    StrategyManager manager;
    auto portfolio = std::make_shared<Portfolio>(1000000.0);
    auto strategy = std::make_shared<SMAStrategy>("wait_test", portfolio, "AAPL", 3, 5, 100);
    
    manager.addStrategy(strategy);
    strategy->start();
    
    // Send only 3 prices (need 5 for slow SMA)
    for (int i = 0; i < 3; ++i) {
        TradeEvent event("AAPL", 100.0 + i, 100);
        EventBus::getInstance().publish(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Should not have opened any position
    auto position = portfolio->getOrderManager()->getPosition("AAPL");
    assert(position == nullptr || position->getQuantity() == 0);
    
    manager.stopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    std::cout << "  ✓ Strategy data waiting passed" << std::endl;
}

#ifndef TEST_ALL
int main() {
    std::cout << "\n=== Strategy Tests ===" << std::endl;
    
    test_strategy_lifecycle();
    test_strategy_manager();
    test_sma_calculation();
    test_sma_crossover_buy_signal();
    test_sma_crossover_sell_signal();
    test_strategy_manager_routing();
    test_strategy_ignores_wrong_symbol();
    test_strategy_waits_for_sufficient_data();
    
    std::cout << "\n✓ All strategy tests passed!\n" << std::endl;
    return 0;
}
#endif
