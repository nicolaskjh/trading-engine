#pragma once

#include "Event.h"

#include <functional>
#include <string>

namespace engine {

/**
 * TimerEvent: Scheduled or periodic events with optional callbacks
 */
class TimerEvent : public Event {
public:
    using TimerCallback = std::function<void()>;

    explicit TimerEvent(const std::string& name, TimerCallback callback = nullptr)
        : Event(EventType::TIMER),
          name_(name),
          callback_(std::move(callback)) {}

    const std::string& getName() const { return name_; }
    
    // Execute the timer's callback
    void execute() const {
        if (callback_) {
            callback_();
        }
    }
    
    bool hasCallback() const { return callback_ != nullptr; }

private:
    std::string name_;
    TimerCallback callback_;
};

/**
 * SystemEventType: Types of system control and status messages
 */
enum class SystemEventType {
    STARTUP,
    SHUTDOWN,
    TRADING_START,
    TRADING_STOP,
    EMERGENCY_STOP,
    CONFIG_RELOAD,
    HEALTH_CHECK,
    CONNECTION_UP,
    CONNECTION_DOWN
};

/**
 * SystemEvent: System control and status messages
 */
class SystemEvent : public Event {
public:
    SystemEvent(SystemEventType sysType, const std::string& message = "")
        : Event(EventType::SYSTEM),
          systemType_(sysType),
          message_(message) {}

    SystemEventType getSystemEventType() const { return systemType_; }
    const std::string& getMessage() const { return message_; }

private:
    SystemEventType systemType_;
    std::string message_;
};

} // namespace engine
