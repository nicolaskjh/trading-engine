#pragma once

#include "Event.h"

#include <string>

namespace engine {

/**
 * QuoteEvent: Represents bid/ask price updates
 */
class QuoteEvent : public Event {
public:
    QuoteEvent(const std::string& symbol, double bidPrice, double askPrice,
               int64_t bidSize, int64_t askSize)
        : Event(EventType::MARKET_DATA),
          symbol_(symbol),
          bidPrice_(bidPrice),
          askPrice_(askPrice),
          bidSize_(bidSize),
          askSize_(askSize) {}

    const std::string& getSymbol() const { return symbol_; }
    double getBidPrice() const { return bidPrice_; }
    double getAskPrice() const { return askPrice_; }
    int64_t getBidSize() const { return bidSize_; }
    int64_t getAskSize() const { return askSize_; }
    double getSpread() const { return askPrice_ - bidPrice_; }
    double getMidPrice() const { return (bidPrice_ + askPrice_) / 2.0; }

private:
    std::string symbol_;
    double bidPrice_;
    double askPrice_;
    int64_t bidSize_;
    int64_t askSize_;
};

/**
 * TradeEvent: Represents executed trade (last sale)
 */
class TradeEvent : public Event {
public:
    TradeEvent(const std::string& symbol, double price, int64_t size)
        : Event(EventType::MARKET_DATA),
          symbol_(symbol),
          price_(price),
          size_(size) {}

    const std::string& getSymbol() const { return symbol_; }
    double getPrice() const { return price_; }
    int64_t getSize() const { return size_; }

private:
    std::string symbol_;
    double price_;
    int64_t size_;
};

} // namespace engine
