#include "data/BookManager.h"
#include "event/EventBus.h"
#include "event/MarketDataEvent.h"
#include "logger/Logger.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace engine;

void testBookManagerBasics() {
    Logger::info(LogComponent::TEST, "=== Testing BookManager Basics ===");
    
    BookManager manager;
    
    // Initial state
    assert(manager.getBookCount() == 0);
    assert(manager.getAllSymbols().empty());
    assert(!manager.hasBook("AAPL"));
    
    // Get non-existent book
    assert(manager.getBook("AAPL") == nullptr);
    
    Logger::info(LogComponent::TEST, "✓ Initial state verified");
}

void testCreateAndAccessBooks() {
    Logger::info(LogComponent::TEST, "=== Testing Book Creation ===");
    
    BookManager manager;
    
    // Create books
    auto* aaplBook = manager.getOrCreateBook("AAPL");
    assert(aaplBook != nullptr);
    assert(aaplBook->getSymbol() == "AAPL");
    assert(manager.getBookCount() == 1);
    assert(manager.hasBook("AAPL"));
    
    auto* tslaBook = manager.getOrCreateBook("TSLA");
    assert(tslaBook != nullptr);
    assert(manager.getBookCount() == 2);
    
    // Get existing book returns same instance
    auto* aaplBook2 = manager.getOrCreateBook("AAPL");
    assert(aaplBook == aaplBook2);
    assert(manager.getBookCount() == 2); // Still 2, not 3
    
    // Test getAllSymbols
    auto symbols = manager.getAllSymbols();
    assert(symbols.size() == 2);
    assert(symbols[0] == "AAPL");  // Sorted alphabetically
    assert(symbols[1] == "TSLA");
    
    std::ostringstream oss;
    oss << "Managed symbols: ";
    for (const auto& sym : symbols) {
        oss << sym << " ";
    }
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Book creation and access working");
}

void testMarketDataIntegration() {
    Logger::info(LogComponent::TEST, "=== Testing Market Data Integration ===");
    
    BookManager manager;
    
    // Publish quote events
    QuoteEvent quote1("AAPL", 150.00, 150.05, 100, 200);
    EventBus::getInstance().publish(quote1);
    EventBus::getInstance().processQueue(10);
    
    // Book should be auto-created
    assert(manager.hasBook("AAPL"));
    auto* book = manager.getBook("AAPL");
    assert(book != nullptr);
    
    // Verify book was updated
    auto bestBid = book->getBestBid();
    auto bestAsk = book->getBestAsk();
    assert(bestBid.has_value());
    assert(bestAsk.has_value());
    assert(bestBid->price == 150.00);
    assert(bestBid->quantity == 100);
    assert(bestAsk->price == 150.05);
    assert(bestAsk->quantity == 200);
    
    Logger::info(LogComponent::TEST, 
                 "AAPL BBO: 150.00 x 100 / 150.05 x 200");
    
    // Publish another quote for different symbol
    QuoteEvent quote2("TSLA", 250.50, 250.60, 300, 150);
    EventBus::getInstance().publish(quote2);
    EventBus::getInstance().processQueue(10);
    
    assert(manager.getBookCount() == 2);
    
    Logger::info(LogComponent::TEST, "✓ Market data integration working");
}

void testTopOfBooks() {
    Logger::info(LogComponent::TEST, "=== Testing Top of Books ===");
    
    BookManager manager;
    
    // Create books with market data
    QuoteEvent quote1("AAPL", 150.00, 150.02, 100, 200);
    QuoteEvent quote2("TSLA", 250.50, 250.55, 300, 150);
    QuoteEvent quote3("GOOGL", 3000.00, 3000.50, 50, 75);
    
    EventBus::getInstance().publish(quote1);
    EventBus::getInstance().publish(quote2);
    EventBus::getInstance().publish(quote3);
    EventBus::getInstance().processQueue(10);
    
    // Get single top of book
    auto aaplTob = manager.getTopOfBook("AAPL");
    assert(aaplTob.has_value());
    assert(aaplTob->symbol == "AAPL");
    assert(aaplTob->bidPrice == 150.00);
    assert(aaplTob->askPrice == 150.02);
    assert(aaplTob->bidSize == 100);
    assert(aaplTob->askSize == 200);
    assert(std::abs(aaplTob->spread - 0.02) < 0.0001);
    assert(std::abs(aaplTob->midPrice - 150.01) < 0.0001);
    
    // Get all tops of books
    auto allTobs = manager.getTopOfBooks();
    assert(allTobs.size() == 3);
    
    std::ostringstream oss;
    oss << "\nAll Top of Books:\n";
    for (const auto& tob : allTobs) {
        oss << "  " << tob.symbol << ": ";
        if (tob.bidPrice && tob.askPrice) {
            oss << std::fixed << std::setprecision(2)
                << *tob.bidPrice << " x " << *tob.bidSize
                << " / " << *tob.askPrice << " x " << *tob.askSize
                << " (Mid: $" << tob.midPrice << ")\n";
        }
    }
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Top of books retrieval working");
}

void testRemoveBooks() {
    Logger::info(LogComponent::TEST, "=== Testing Remove Books ===");
    
    BookManager manager;
    
    // Create books
    manager.getOrCreateBook("AAPL");
    manager.getOrCreateBook("TSLA");
    manager.getOrCreateBook("GOOGL");
    
    assert(manager.getBookCount() == 3);
    
    // Remove one book
    manager.removeBook("TSLA");
    assert(manager.getBookCount() == 2);
    assert(!manager.hasBook("TSLA"));
    assert(manager.hasBook("AAPL"));
    assert(manager.hasBook("GOOGL"));
    
    // Remove non-existent book (should not crash)
    manager.removeBook("NVDA");
    assert(manager.getBookCount() == 2);
    
    // Clear all books
    manager.clearAllBooks();
    assert(manager.getBookCount() == 0);
    assert(manager.getAllSymbols().empty());
    
    Logger::info(LogComponent::TEST, "✓ Book removal working correctly");
}

void testTradeEventHandling() {
    Logger::info(LogComponent::TEST, "=== Testing Trade Event Handling ===");
    
    BookManager manager;
    
    // Publish trade event for new symbol
    TradeEvent trade1("NVDA", 500.00, 1000);
    EventBus::getInstance().publish(trade1);
    EventBus::getInstance().processQueue(10);
    
    // Book should be created but empty (trades don't update levels)
    assert(manager.hasBook("NVDA"));
    auto* book = manager.getBook("NVDA");
    assert(book->isEmpty());
    
    // Now publish a quote
    QuoteEvent quote1("NVDA", 499.50, 500.50, 200, 300);
    EventBus::getInstance().publish(quote1);
    EventBus::getInstance().processQueue(10);
    
    // Book should now have data
    assert(!book->isEmpty());
    assert(book->getBestBid().has_value());
    
    Logger::info(LogComponent::TEST, "✓ Trade event handling working");
}

void testMultipleQuoteUpdates() {
    Logger::info(LogComponent::TEST, "=== Testing Multiple Quote Updates ===");
    
    BookManager manager;
    
    // Send multiple updates for same symbol
    QuoteEvent quote1("AAPL", 150.00, 150.10, 100, 200);
    QuoteEvent quote2("AAPL", 150.05, 150.08, 150, 180);
    QuoteEvent quote3("AAPL", 150.03, 150.09, 120, 160);
    
    EventBus::getInstance().publish(quote1);
    EventBus::getInstance().publish(quote2);
    EventBus::getInstance().publish(quote3);
    EventBus::getInstance().processQueue(10);
    
    // Book should reflect latest quote
    auto* book = manager.getBook("AAPL");
    auto bestBid = book->getBestBid();
    auto bestAsk = book->getBestAsk();
    
    assert(bestBid->price == 150.05);  // From quote2
    assert(bestBid->quantity == 120);  // From quote3 (highest bid)
    
    // Book should have multiple levels
    assert(book->getBidLevelCount() == 3);
    assert(book->getAskLevelCount() == 3);
    
    auto bidDepth = book->getBidDepth(3);
    std::ostringstream oss;
    oss << "\nAAPL Bid Depth after updates:\n";
    for (const auto& level : bidDepth) {
        oss << "  $" << std::fixed << std::setprecision(2)
            << level.price << " x " << level.quantity << "\n";
    }
    Logger::info(LogComponent::TEST, oss.str());
    
    Logger::info(LogComponent::TEST, "✓ Multiple quote updates working");
}

#ifndef TEST_ALL
int main() {
    Logger::init(LogLevel::INFO);
    
    try {
        Logger::info(LogComponent::TEST, "Starting BookManager Tests...\n");
        
        testBookManagerBasics();
        testCreateAndAccessBooks();
        testMarketDataIntegration();
        testTopOfBooks();
        testRemoveBooks();
        testTradeEventHandling();
        testMultipleQuoteUpdates();
        
        Logger::info(LogComponent::TEST, "\n✓ All BookManager tests passed!");
        
    } catch (const std::exception& e) {
        Logger::critical(LogComponent::TEST, "Test failed: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
#endif
