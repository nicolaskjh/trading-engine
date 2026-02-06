#pragma once

#include "event/EventBus.h"
#include "market_data/OrderBook.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

/**
 * Manages order books for multiple symbols
 * Subscribes to market data events and routes them to appropriate books
 */
class BookManager {
public:
    BookManager();
    ~BookManager();
    
    // Book access
    OrderBook* getBook(const std::string& symbol);
    OrderBook* getOrCreateBook(const std::string& symbol);
    bool hasBook(const std::string& symbol) const;
    void removeBook(const std::string& symbol);
    void clearAllBooks();
    
    // Query all symbols
    std::vector<std::string> getAllSymbols() const;
    size_t getBookCount() const { return books_.size(); }
    
    // Top of book across all symbols
    struct TopOfBook {
        std::string symbol;
        std::optional<double> bidPrice;
        std::optional<double> askPrice;
        std::optional<int64_t> bidSize;
        std::optional<int64_t> askSize;
        double spread;
        double midPrice;
    };
    
    std::vector<TopOfBook> getTopOfBooks() const;
    std::optional<TopOfBook> getTopOfBook(const std::string& symbol) const;
    
private:
    // Event handlers
    void onMarketData(const Event& event);
    
    // Book storage
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> books_;
    
    // Event subscription
    size_t subId_;
};

} // namespace engine
