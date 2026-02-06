#pragma once

#include "config/Config.h"
#include "event/EventBus.h"
#include "event/OrderEvent.h"
#include "exchange/ExchangeConnector.h"

#include <mutex>
#include <random>
#include <string>
#include <unordered_map>

namespace engine {

/**
 * SimulatedExchange: Exchange simulator for testing and backtesting.
 * 
 * Simulates realistic exchange behavior including:
 * - Order acceptance/rejection
 * - Fill latency
 * - Partial fills
 * - Slippage for market orders
 * - Order book price levels
 */
class SimulatedExchange : public ExchangeConnector {
public:
    /**
     * Configuration for simulation behavior
     */
    struct Config {
        int fillLatencyMs;
        double rejectionRate;
        double partialFillRate;
        double slippageBps;
        bool instantFills;

        Config();
    };

    explicit SimulatedExchange(const Config& config = Config());
    ~SimulatedExchange();

    void start() override;
    void stop() override;
    bool isRunning() const override;

    void submitOrder(const std::string& orderId,
                    const std::string& symbol,
                    Side side,
                    OrderType type,
                    double price,
                    int64_t quantity) override;

    void cancelOrder(const std::string& orderId) override;

    /**
     * Set current market price for a symbol (for slippage calculation)
     */
    void setMarketPrice(const std::string& symbol, double price);

    /**
     * Get configuration
     */
    const Config& getConfig() const;

    /**
     * Update configuration
     */
    void setConfig(const Config& config);

private:
    struct PendingOrder {
        std::string orderId;
        std::string symbol;
        Side side;
        OrderType type;
        double price;
        int64_t quantity;
    };

    void onOrderEvent(const Event& event);
    void processFill(const std::string& orderId,
                    const std::string& symbol,
                    Side side,
                    OrderType type,
                    double price,
                    int64_t quantity);
    void checkLimitOrders(const std::string& symbol);
    bool shouldReject();
    bool shouldPartialFill();
    double applySlippage(const std::string& symbol, Side side, double price);

    Config config_;
    bool isRunning_;
    uint64_t orderSubId_;

    std::unordered_map<std::string, PendingOrder> pendingOrders_;
    std::unordered_map<std::string, double> marketPrices_;

    std::mt19937 randomEngine_;
    mutable std::mutex mutex_;
};

} // namespace engine
