#include "logger/Logger.h"

#include <iostream>

using namespace engine;

// Forward declarations
int testEventSystem();
int testOrderManager();
int testOrderBook();
int testBookManager();

// From test_event.cpp
extern void testEventPublishSubscribe();
extern void testSystemEvents();
extern void testTimerEvents();
extern void testEventCount();

// From test_ordermanager.cpp
extern void testOrderSubmission();
extern void testOrderLifecycle();
extern void testPositionTracking();
extern void testPnLCalculation();
extern void testMultiplePositions();

// From test_orderbook.cpp
extern void testOrderBookBasics();
extern void testBidAskUpdates();
extern void testSpreadAndMidPrice();
extern void testLevelUpdates();
extern void testBookDepth();
extern void testCrossedBook();
extern void testClearBook();

// From test_bookmanager.cpp
extern void testBookManagerBasics();
extern void testCreateAndAccessBooks();
extern void testMarketDataIntegration();
extern void testTopOfBooks();
extern void testRemoveBooks();
extern void testTradeEventHandling();
extern void testMultipleQuoteUpdates();

int testEventSystem() {
    try {
        Logger::info(LogComponent::TEST, "\n========== Event System Tests ==========");
        testEventPublishSubscribe();
        testSystemEvents();
        testTimerEvents();
        testEventCount();
        Logger::info(LogComponent::TEST, "✓ Event System: ALL PASSED\n");
        return 0;
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Event System test failed: " + std::string(e.what()));
        return 1;
    }
}

int testOrderManager() {
    try {
        Logger::info(LogComponent::TEST, "\n========== OrderManager Tests ==========");
        testOrderSubmission();
        testOrderLifecycle();
        testPositionTracking();
        testPnLCalculation();
        testMultiplePositions();
        Logger::info(LogComponent::TEST, "✓ OrderManager: ALL PASSED\n");
        return 0;
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "OrderManager test failed: " + std::string(e.what()));
        return 1;
    }
}

int testOrderBook() {
    try {
        Logger::info(LogComponent::TEST, "\n========== OrderBook Tests ==========");
        testOrderBookBasics();
        testBidAskUpdates();
        testSpreadAndMidPrice();
        testLevelUpdates();
        testBookDepth();
        testCrossedBook();
        testClearBook();
        Logger::info(LogComponent::TEST, "✓ OrderBook: ALL PASSED\n");
        return 0;
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "OrderBook test failed: " + std::string(e.what()));
        return 1;
    }
}

int testBookManager() {
    try {
        Logger::info(LogComponent::TEST, "\n========== BookManager Tests ==========");
        testBookManagerBasics();
        testCreateAndAccessBooks();
        testMarketDataIntegration();
        testTopOfBooks();
        testRemoveBooks();
        testTradeEventHandling();
        testMultipleQuoteUpdates();
        Logger::info(LogComponent::TEST, "✓ BookManager: ALL PASSED\n");
        return 0;
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "BookManager test failed: " + std::string(e.what()));
        return 1;
    }
}

int main() {
    Logger::init(LogLevel::INFO);
    
    std::cout << "========================================\n";
    std::cout << "   Trading Engine - All Tests\n";
    std::cout << "========================================\n";
    
    int failures = 0;
    
    failures += testEventSystem();
    failures += testOrderManager();
    failures += testOrderBook();
    failures += testBookManager();
    
    std::cout << "\n========================================\n";
    if (failures == 0) {
        std::cout << "   ✓ ALL TESTS PASSED\n";
    } else {
        std::cout << "   ✗ " << failures << " TEST SUITE(S) FAILED\n";
    }
    std::cout << "========================================\n";
    
    Logger::shutdown();
    return failures;
}
