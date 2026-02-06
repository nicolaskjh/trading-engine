#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace engine {

/**
 * Represents a single price level in the order book
 */
struct PriceLevel {
    double price;
    int64_t quantity;
    
    PriceLevel(double p = 0.0, int64_t q = 0) 
        : price(p), quantity(q) {}
};

/**
 * Order Book for a single symbol
 * Maintains bid and ask sides with aggregated price levels
 * Optimized for fast BBO (Best Bid/Offer) access
 */
class OrderBook {
public:
    explicit OrderBook(const std::string& symbol);
    
    // Book updates
    void updateBid(double price, int64_t quantity);
    void updateAsk(double price, int64_t quantity);
    void removeBid(double price);
    void removeAsk(double price);
    void clear();
    
    // Best bid/offer access
    std::optional<PriceLevel> getBestBid() const;
    std::optional<PriceLevel> getBestAsk() const;
    
    // Market metrics
    double getSpread() const;
    double getMidPrice() const;
    std::optional<double> getBestBidPrice() const;
    std::optional<double> getBestAskPrice() const;
    
    // Book depth
    std::vector<PriceLevel> getBidDepth(size_t levels = 10) const;
    std::vector<PriceLevel> getAskDepth(size_t levels = 10) const;
    
    // State queries
    const std::string& getSymbol() const { return symbol_; }
    size_t getBidLevelCount() const { return bids_.size(); }
    size_t getAskLevelCount() const { return asks_.size(); }
    bool isEmpty() const { return bids_.empty() && asks_.empty(); }
    
private:
    std::string symbol_;
    
    // Price levels: price -> quantity
    // Bids: std::greater for descending order (highest price first)
    // Asks: std::less for ascending order (lowest price first)
    std::map<double, int64_t, std::greater<double>> bids_;
    std::map<double, int64_t, std::less<double>> asks_;
};

} // namespace engine
