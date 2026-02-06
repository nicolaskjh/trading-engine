#pragma once

#include "data/OrderBook.h"
#include "event/Event.h"
#include "event/MarketDataEvent.h"
#include "event/OrderEvent.h"
#include "risk/Portfolio.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace engine {

/**
 * Strategy: Abstract base class for trading strategies.
 * 
 * Provides lifecycle hooks and access to market data and portfolio.
 * Subclasses implement trading logic by overriding the virtual methods.
 */
class Strategy {
public:
    /**
     * Constructor with strategy name and portfolio
     */
    Strategy(const std::string& name, std::shared_ptr<Portfolio> portfolio)
        : name_(name)
        , portfolio_(portfolio)
        , isRunning_(false)
    {}

    virtual ~Strategy() = default;

    // Prevent copying
    Strategy(const Strategy&) = delete;
    Strategy& operator=(const Strategy&) = delete;

    /**
     * Get strategy name
     */
    const std::string& getName() const {
        return name_;
    }

    /**
     * Check if strategy is running
     */
    bool isRunning() const {
        return isRunning_;
    }

    /**
     * Start the strategy (called once at initialization)
     */
    void start() {
        if (!isRunning_) {
            isRunning_ = true;
            onStart();
        }
    }

    /**
     * Stop the strategy (called at shutdown)
     */
    void stop() {
        if (isRunning_) {
            isRunning_ = false;
            onStop();
        }
    }

    /**
     * Handle trade event (market data)
     */
    void handleTradeEvent(const TradeEvent& event) {
        if (isRunning_) {
            onTradeEvent(event);
        }
    }

    /**
     * Handle quote event (market data)
     */
    void handleQuoteEvent(const QuoteEvent& event) {
        if (isRunning_) {
            onQuoteEvent(event);
        }
    }

    /**
     * Handle order event (status updates)
     */
    void handleOrderEvent(const OrderEvent& event) {
        if (isRunning_) {
            onOrderEvent(event);
        }
    }

    /**
     * Handle fill event
     */
    void handleFillEvent(const FillEvent& event) {
        if (isRunning_) {
            onFillEvent(event);
        }
    }

protected:
    /**
     * Called when strategy starts - override to initialize state
     */
    virtual void onStart() {}

    /**
     * Called when strategy stops - override to cleanup
     */
    virtual void onStop() {}

    /**
     * Called on trade events - override to implement trading logic
     */
    virtual void onTradeEvent(const TradeEvent& event) {}

    /**
     * Called on quote events - override to implement trading logic
     */
    virtual void onQuoteEvent(const QuoteEvent& event) {}

    /**
     * Called on order status updates - override to track order lifecycle
     */
    virtual void onOrderEvent(const OrderEvent& event) {}

    /**
     * Called on fill events - override to update strategy state
     */
    virtual void onFillEvent(const FillEvent& event) {}

    /**
     * Submit order through portfolio (with risk checks)
     */
    bool submitOrder(const std::string& orderId, const std::string& symbol,
                    Side side, OrderType type, double price, int64_t quantity,
                    const std::unordered_map<std::string, double>& marketPrices) {
        return portfolio_->submitOrder(orderId, symbol, side, type, price, quantity, marketPrices);
    }

    /**
     * Cancel an order
     */
    void cancelOrder(const std::string& orderId) {
        portfolio_->cancelOrder(orderId);
    }

    /**
     * Get position for a symbol
     */
    std::shared_ptr<Position> getPosition(const std::string& symbol) const {
        return portfolio_->getOrderManager()->getPosition(symbol);
    }

    /**
     * Access to portfolio
     */
    Portfolio* getPortfolio() {
        return portfolio_.get();
    }

    const Portfolio* getPortfolio() const {
        return portfolio_.get();
    }

    /**
     * Generate unique order ID
     */
    std::string generateOrderId() {
        return name_ + "_" + std::to_string(++orderCounter_);
    }

private:
    std::string name_;
    std::shared_ptr<Portfolio> portfolio_;
    bool isRunning_;
    uint64_t orderCounter_ = 0;
};

} // namespace engine
