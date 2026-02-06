#include "data/BookManager.h"
#include "event/MarketDataEvent.h"
#include "logger/Logger.h"

#include <algorithm>

namespace engine {

BookManager::BookManager() {
    subId_ = EventBus::getInstance().subscribe(
        EventType::MARKET_DATA,
        [this](const Event& event) { this->onMarketData(event); }
    );
    
    Logger::info(LogComponent::MARKET_DATA_HANDLER, "BookManager initialized");
}

BookManager::~BookManager() {
    EventBus::getInstance().unsubscribe(subId_);
}

OrderBook* BookManager::getBook(const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        return nullptr;
    }
    return it->second.get();
}

OrderBook* BookManager::getOrCreateBook(const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it != books_.end()) {
        return it->second.get();
    }
    
    // Create new book
    auto book = std::make_unique<OrderBook>(symbol);
    auto* bookPtr = book.get();
    books_[symbol] = std::move(book);
    
    Logger::info(LogComponent::MARKET_DATA_HANDLER, 
                 "Created order book for " + symbol);
    
    return bookPtr;
}

bool BookManager::hasBook(const std::string& symbol) const {
    return books_.find(symbol) != books_.end();
}

void BookManager::removeBook(const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it != books_.end()) {
        books_.erase(it);
        Logger::info(LogComponent::MARKET_DATA_HANDLER, 
                     "Removed order book for " + symbol);
    }
}

void BookManager::clearAllBooks() {
    books_.clear();
    Logger::info(LogComponent::MARKET_DATA_HANDLER, "Cleared all order books");
}

std::vector<std::string> BookManager::getAllSymbols() const {
    std::vector<std::string> symbols;
    symbols.reserve(books_.size());
    
    for (const auto& [symbol, book] : books_) {
        symbols.push_back(symbol);
    }
    
    std::sort(symbols.begin(), symbols.end());
    return symbols;
}

std::vector<BookManager::TopOfBook> BookManager::getTopOfBooks() const {
    std::vector<TopOfBook> result;
    result.reserve(books_.size());
    
    for (const auto& [symbol, book] : books_) {
        auto tob = getTopOfBook(symbol);
        if (tob) {
            result.push_back(*tob);
        }
    }
    
    // Sort by symbol
    std::sort(result.begin(), result.end(),
              [](const TopOfBook& a, const TopOfBook& b) {
                  return a.symbol < b.symbol;
              });
    
    return result;
}

std::optional<BookManager::TopOfBook> BookManager::getTopOfBook(
    const std::string& symbol) const {
    
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        return std::nullopt;
    }
    
    const auto* book = it->second.get();
    auto bestBid = book->getBestBid();
    auto bestAsk = book->getBestAsk();
    
    TopOfBook tob;
    tob.symbol = symbol;
    
    if (bestBid) {
        tob.bidPrice = bestBid->price;
        tob.bidSize = bestBid->quantity;
    }
    
    if (bestAsk) {
        tob.askPrice = bestAsk->price;
        tob.askSize = bestAsk->quantity;
    }
    
    tob.spread = book->getSpread();
    tob.midPrice = book->getMidPrice();
    
    return tob;
}

void BookManager::onMarketData(const Event& event) {
    // Handle QuoteEvent
    if (const auto* quote = dynamic_cast<const QuoteEvent*>(&event)) {
        auto* book = getOrCreateBook(quote->getSymbol());
        
        // Update both sides of the book
        book->updateBid(quote->getBidPrice(), quote->getBidSize());
        book->updateAsk(quote->getAskPrice(), quote->getAskSize());
        
        return;
    }
    
    // TradeEvents don't directly update the order book
    // They could be used for trade analytics, volume tracking, etc.
    // For now, we just acknowledge them
    if (const auto* trade = dynamic_cast<const TradeEvent*>(&event)) {
        // Ensure the book exists for this symbol
        getOrCreateBook(trade->getSymbol());
    }
}

} // namespace engine
