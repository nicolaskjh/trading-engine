#include "logger/Logger.h"
#include "market_data/OrderBook.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace engine;

void testOrderBookBasics() {
    Logger::info(LogComponent::TEST, "=== Testing Order Book Basics ===");
    
    OrderBook book("AAPL");
    
    // Verify initial state
    assert(book.getSymbol() == "AAPL");
    assert(book.isEmpty());
    assert(book.getBidLevelCount() == 0);
    assert(book.getAskLevelCount() == 0);
    assert(!book.getBestBid().has_value());
    assert(!book.getBestAsk().has_value());
    assert(std::isnan(book.getSpread()));
    assert(std::isnan(book.getMidPrice()));
    
    Logger::info(LogComponent::TEST, "✓ Initial state verified");
}

void testBidAskUpdates() {
    Logger::info(LogComponent::TEST, "=== Testing Bid/Ask Updates ===");
    
    OrderBook book("AAPL");
    
    // Add bids
    book.updateBid(150.00, 100);
    book.updateBid(149.99, 200);
    book.updateBid(149.98, 150);
    
    // Add asks
    book.updateAsk(150.01, 100);
    book.updateAsk(150.02, 200);
    book.updateAsk(150.03, 150);
    
    assert(book.getBidLevelCount() == 3);
    assert(book.getAskLevelCount() == 3);
    
    // Verify best bid (highest price)
    auto bestBid = book.getBestBid();
    assert(bestBid.has_value());
    assert(bestBid->price == 150.00);
    assert(bestBid->quantity == 100);
    
    // Verify best ask (lowest price)
    auto bestAsk = book.getBestAsk();
    assert(bestAsk.has_value());
    assert(bestAsk->price == 150.01);
    assert(bestAsk->quantity == 100);
    
    std::ostringstream oss;
    oss << "BBO: " << std::fixed << std::setprecision(2)
        << bestBid->price << " x " << bestBid->quantity
        << " / " << bestAsk->price << " x " << bestAsk->quantity;
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Bid/Ask updates working correctly");
}

void testSpreadAndMidPrice() {
    Logger::info(LogComponent::TEST, "=== Testing Spread and Mid Price ===");
    
    OrderBook book("TSLA");
    
    book.updateBid(250.50, 500);
    book.updateAsk(250.55, 300);
    
    double spread = book.getSpread();
    double midPrice = book.getMidPrice();
    
    assert(std::abs(spread - 0.05) < 0.0001);
    assert(std::abs(midPrice - 250.525) < 0.0001);
    
    std::ostringstream oss;
    oss << "Spread: $" << std::fixed << std::setprecision(4) << spread
        << " | Mid Price: $" << midPrice;
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Spread and mid price calculations correct");
}

void testLevelUpdates() {
    Logger::info(LogComponent::TEST, "=== Testing Level Updates ===");
    
    OrderBook book("MSFT");
    
    // Add initial level
    book.updateBid(300.00, 100);
    assert(book.getBestBid()->quantity == 100);
    
    // Update same level with new quantity
    book.updateBid(300.00, 250);
    assert(book.getBestBid()->quantity == 250);
    assert(book.getBidLevelCount() == 1);
    
    // Remove level with zero quantity
    book.updateBid(300.00, 0);
    assert(!book.getBestBid().has_value());
    assert(book.getBidLevelCount() == 0);
    
    Logger::info(LogComponent::TEST, "✓ Level updates and removals working");
}

void testBookDepth() {
    Logger::info(LogComponent::TEST, "=== Testing Book Depth ===");
    
    OrderBook book("GOOGL");
    
    // Add 5 bid levels
    book.updateBid(3000.00, 100);
    book.updateBid(2999.50, 200);
    book.updateBid(2999.00, 150);
    book.updateBid(2998.50, 300);
    book.updateBid(2998.00, 250);
    
    // Add 5 ask levels
    book.updateAsk(3000.50, 100);
    book.updateAsk(3001.00, 200);
    book.updateAsk(3001.50, 150);
    book.updateAsk(3002.00, 300);
    book.updateAsk(3002.50, 250);
    
    // Get top 3 levels
    auto bidDepth = book.getBidDepth(3);
    auto askDepth = book.getAskDepth(3);
    
    assert(bidDepth.size() == 3);
    assert(askDepth.size() == 3);
    
    // Verify bid levels are in descending order
    assert(bidDepth[0].price == 3000.00);
    assert(bidDepth[1].price == 2999.50);
    assert(bidDepth[2].price == 2999.00);
    
    // Verify ask levels are in ascending order
    assert(askDepth[0].price == 3000.50);
    assert(askDepth[1].price == 3001.00);
    assert(askDepth[2].price == 3001.50);
    
    std::ostringstream oss;
    oss << "\nTop 3 Bids:\n";
    for (const auto& level : bidDepth) {
        oss << "  $" << std::fixed << std::setprecision(2) 
            << level.price << " x " << level.quantity << "\n";
    }
    oss << "Top 3 Asks:\n";
    for (const auto& level : askDepth) {
        oss << "  $" << std::fixed << std::setprecision(2) 
            << level.price << " x " << level.quantity << "\n";
    }
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Book depth retrieval working correctly");
}

void testCrossedBook() {
    Logger::info(LogComponent::TEST, "=== Testing Crossed Book Detection ===");
    
    OrderBook book("AMD");
    
    // Normal book
    book.updateBid(100.00, 100);
    book.updateAsk(100.10, 100);
    
    double spread1 = book.getSpread();
    assert(spread1 > 0);
    
    Logger::info(LogComponent::TEST, "Normal spread: $" + std::to_string(spread1));
    
    // Crossed book (bid > ask - shouldn't happen in real market)
    book.updateBid(100.20, 100);
    
    double spread2 = book.getSpread();
    assert(spread2 < 0); // Negative spread indicates crossed book
    
    Logger::info(LogComponent::TEST, "Crossed spread: $" + std::to_string(spread2));
    Logger::info(LogComponent::TEST, "✓ Can detect crossed book condition");
}

void testClearBook() {
    Logger::info(LogComponent::TEST, "=== Testing Clear Book ===");
    
    OrderBook book("NVDA");
    
    // Populate book
    book.updateBid(500.00, 100);
    book.updateBid(499.50, 200);
    book.updateAsk(500.50, 100);
    book.updateAsk(501.00, 200);
    
    assert(!book.isEmpty());
    assert(book.getBidLevelCount() == 2);
    assert(book.getAskLevelCount() == 2);
    
    // Clear book
    book.clear();
    
    assert(book.isEmpty());
    assert(book.getBidLevelCount() == 0);
    assert(book.getAskLevelCount() == 0);
    assert(!book.getBestBid().has_value());
    assert(!book.getBestAsk().has_value());
    
    Logger::info(LogComponent::TEST, "✓ Book cleared successfully");
}

#ifndef TEST_ALL
int main() {
    Logger::init(LogLevel::INFO);
    
    try {
        Logger::info(LogComponent::TEST, "Starting Order Book Tests...\n");
        
        testOrderBookBasics();
        testBidAskUpdates();
        testSpreadAndMidPrice();
        testLevelUpdates();
        testBookDepth();
        testCrossedBook();
        testClearBook();
        
        Logger::info(LogComponent::TEST, "\n✓ All Order Book tests passed!");
        
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Test failed: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
#endif
