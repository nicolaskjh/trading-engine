#pragma once

#include "Event.h"

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

namespace engine {

/**
 * EventHandler: Type alias for event callback functions
 */
using EventHandler = std::function<void(const Event&)>;

/**
 * EventBus: Central event dispatcher using publish-subscribe pattern.
 * Singleton pattern for centralized event flow. Supports both synchronous
 * dispatch and asynchronous queue processing.
 */
class EventBus {
public:
    // Get singleton instance
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    // Delete copy/move constructors for singleton
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    /**
     * Subscribe to events of a specific type
     * @param type EventType to listen for
     * @param handler Callback function to invoke when event occurs
     * @return Subscription ID (can be used to unsubscribe later)
     */
    uint64_t subscribe(EventType type, EventHandler handler) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        uint64_t id = nextSubscriptionId_++;
        subscribers_[type].push_back({id, std::move(handler)});
        return id;
    }

    /**
     * Unsubscribe from events
     * @param subscriptionId ID returned from subscribe()
     */
    void unsubscribe(uint64_t subscriptionId) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& [type, handlers] : subscribers_) {
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [subscriptionId](const SubscriptionPair& sub) {
                        return sub.id == subscriptionId;
                    }),
                handlers.end()
            );
        }
    }

    /**
     * Publish event synchronously - handlers called immediately
     * This is the primary method for time-critical events
     */
    void publish(const Event& event) {
        // Copy handlers while holding lock, then release before calling them
        std::vector<EventHandler> handlersCopy;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            
            auto it = subscribers_.find(event.getType());
            if (it != subscribers_.end()) {
                // Copy handlers to avoid holding lock during callback execution
                for (const auto& sub : it->second) {
                    handlersCopy.push_back(sub.handler);
                }
            }
            
            ++eventCount_;
        }
        
        // Call handlers without holding lock to avoid deadlock if they publish events
        for (const auto& handler : handlersCopy) {
            handler(event);
        }
    }

    /**
     * Publish event by moving ownership (zero-copy)
     * Useful when event is created solely for publishing
     */
    void publish(EventPtr event) {
        if (event) {
            publish(*event);
        }
    }

    /**
     * Enqueue event for async processing
     * Use for non-critical events (logging, metrics)
     */
    void enqueue(EventPtr event) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        eventQueue_.push(std::move(event));
    }

    /**
     * Process queued events (call periodically from event loop)
     * @param maxEvents Maximum number of events to process (0 = all)
     */
    void processQueue(size_t maxEvents = 0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        size_t processed = 0;
        while (!eventQueue_.empty() && (maxEvents == 0 || processed < maxEvents)) {
            EventPtr event = std::move(eventQueue_.front());
            eventQueue_.pop();
            
            // Process without holding lock on subscribers
            auto it = subscribers_.find(event->getType());
            if (it != subscribers_.end()) {
                for (const auto& sub : it->second) {
                    sub.handler(*event);
                }
            }
            
            ++processed;
        }
    }

    // Statistics
    uint64_t getEventCount() const { return eventCount_; }
    size_t getQueueSize() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return eventQueue_.size();
    }
    
    // Clear all subscribers (useful for testing/shutdown)
    void clear() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        subscribers_.clear();
        while (!eventQueue_.empty()) eventQueue_.pop();
        eventCount_ = 0;
    }

private:
    EventBus() : nextSubscriptionId_(1), eventCount_(0) {}
    ~EventBus() = default;

    // Subscription management
    struct SubscriptionPair {
        uint64_t id;
        EventHandler handler;
        
        SubscriptionPair(uint64_t i, EventHandler h) 
            : id(i), handler(std::move(h)) {}
    };
    
    std::unordered_map<EventType, std::vector<SubscriptionPair>> subscribers_;
    uint64_t nextSubscriptionId_;
    
    // Async event queue
    std::queue<EventPtr> eventQueue_;
    
    // Statistics
    uint64_t eventCount_;
    
    // Thread safety
    mutable std::recursive_mutex mutex_;
};

} // namespace engine
