#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

namespace engine {

/**
 * EventType: Categorizes events in the system
 */
enum class EventType {
    MARKET_DATA,    // Price updates, order book changes
    ORDER,          // Order lifecycle events
    FILL,           // Execution reports
    TIMER,          // Scheduled/periodic events
    SYSTEM,         // Control messages, shutdown signals
    RISK            // Risk violations, warnings
};

/**
 * Event: Base class for all events in the trading system.
 * Events are timestamped at creation and non-copyable for performance.
 */
class Event {
public:
    explicit Event(EventType type)
        : type_(type),
          timestamp_(std::chrono::high_resolution_clock::now()) {}

    virtual ~Event() = default;

    // Prevent copying - events should be moved or passed by reference
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    // Allow moving
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    EventType getType() const { return type_; }
    
    auto getTimestamp() const { return timestamp_; }

    // Get age of event in microseconds (useful for latency monitoring)
    int64_t getAgeInMicroseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now - timestamp_).count();
    }

private:
    EventType type_;
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp_;
};

// Type alias for event pointers
using EventPtr = std::unique_ptr<Event>;

} // namespace engine
