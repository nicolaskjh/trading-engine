#pragma once

#include "event/OrderEvent.h"

#include <string>

namespace engine {

/**
 * ExchangeConnector: Abstract interface for exchange connectivity.
 * 
 * Defines the contract that all exchange implementations must follow,
 * whether simulated or real (Binance, Coinbase, etc.).
 */
class ExchangeConnector {
public:
    virtual ~ExchangeConnector() = default;

    /**
     * Start the exchange connector (connect, authenticate, subscribe to events)
     */
    virtual void start() = 0;

    /**
     * Stop the exchange connector (disconnect, cleanup)
     */
    virtual void stop() = 0;

    /**
     * Check if connector is running
     */
    virtual bool isRunning() const = 0;

    /**
     * Submit an order to the exchange
     * The exchange will publish ORDER and FILL events asynchronously
     */
    virtual void submitOrder(const std::string& orderId,
                            const std::string& symbol,
                            Side side,
                            OrderType type,
                            double price,
                            int64_t quantity) = 0;

    /**
     * Cancel an order at the exchange
     */
    virtual void cancelOrder(const std::string& orderId) = 0;

protected:
    ExchangeConnector() = default;
};

} // namespace engine
