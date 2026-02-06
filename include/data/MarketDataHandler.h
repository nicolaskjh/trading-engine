#pragma once

#include "event/EventBus.h"
#include "event/MarketDataEvent.h"

namespace engine {

/**
 * MarketDataHandler: Processes and logs incoming market data events.
 * Subscribes to MARKET_DATA events and outputs quote and trade information.
 */
class MarketDataHandler {
public:
    MarketDataHandler();
    ~MarketDataHandler();

    // Prevent copying
    MarketDataHandler(const MarketDataHandler&) = delete;
    MarketDataHandler& operator=(const MarketDataHandler&) = delete;

private:
    void onMarketData(const Event& event);
    uint64_t subId_;
};

} // namespace engine
