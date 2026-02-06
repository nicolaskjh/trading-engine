#include "data/OrderBook.h"

#include <cmath>
#include <limits>

namespace engine {

OrderBook::OrderBook(const std::string& symbol) 
    : symbol_(symbol) {
}

void OrderBook::updateBid(double price, int64_t quantity) {
    if (quantity > 0) {
        bids_[price] = quantity;
    } else {
        // Zero quantity means remove the level
        bids_.erase(price);
    }
}

void OrderBook::updateAsk(double price, int64_t quantity) {
    if (quantity > 0) {
        asks_[price] = quantity;
    } else {
        // Zero quantity means remove the level
        asks_.erase(price);
    }
}

void OrderBook::removeBid(double price) {
    bids_.erase(price);
}

void OrderBook::removeAsk(double price) {
    asks_.erase(price);
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
}

std::optional<PriceLevel> OrderBook::getBestBid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    
    // First element is best bid (highest price) due to std::greater
    const auto& [price, quantity] = *bids_.begin();
    return PriceLevel{price, quantity};
}

std::optional<PriceLevel> OrderBook::getBestAsk() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    
    // First element is best ask (lowest price) due to std::less
    const auto& [price, quantity] = *asks_.begin();
    return PriceLevel{price, quantity};
}

double OrderBook::getSpread() const {
    auto bestBid = getBestBidPrice();
    auto bestAsk = getBestAskPrice();
    
    if (!bestBid || !bestAsk) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return *bestAsk - *bestBid;
}

double OrderBook::getMidPrice() const {
    auto bestBid = getBestBidPrice();
    auto bestAsk = getBestAskPrice();
    
    if (!bestBid || !bestAsk) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*bestBid + *bestAsk) / 2.0;
}

std::optional<double> OrderBook::getBestBidPrice() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> OrderBook::getBestAskPrice() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::vector<PriceLevel> OrderBook::getBidDepth(size_t levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(std::min(levels, bids_.size()));
    
    size_t count = 0;
    for (const auto& [price, quantity] : bids_) {
        if (count >= levels) break;
        depth.emplace_back(price, quantity);
        ++count;
    }
    
    return depth;
}

std::vector<PriceLevel> OrderBook::getAskDepth(size_t levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(std::min(levels, asks_.size()));
    
    size_t count = 0;
    for (const auto& [price, quantity] : asks_) {
        if (count >= levels) break;
        depth.emplace_back(price, quantity);
        ++count;
    }
    
    return depth;
}

} // namespace engine
