#pragma once

#include "event/EventBus.h"
#include "event/OrderEvent.h"

namespace engine {

/**
 * OrderLogger: Logs order lifecycle and fill events for monitoring.
 * Subscribes to ORDER and FILL events and outputs detailed information.
 */
class OrderLogger {
public:
    OrderLogger();
    ~OrderLogger();

    // Prevent copying
    OrderLogger(const OrderLogger&) = delete;
    OrderLogger& operator=(const OrderLogger&) = delete;

private:
    void onOrderEvent(const Event& event);
    void onFillEvent(const Event& event);
    
    uint64_t orderSubId_;
    uint64_t fillSubId_;
};

} // namespace engine
