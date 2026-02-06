#include "EventBus.h"
#include "MarketDataEvent.h"
#include "OrderEvent.h"
#include "TimerEvent.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace engine;

/**
 * MarketDataHandler: Processes incoming market data events
 */
class MarketDataHandler {
public:
    MarketDataHandler() {
        // Subscribe to market data events
        subId_ = EventBus::getInstance().subscribe(
            EventType::MARKET_DATA,
            [this](const Event& event) { this->onMarketData(event); }
        );
        
        std::cout << "[MarketDataHandler] Initialized\n";
    }
    
    ~MarketDataHandler() {
        EventBus::getInstance().unsubscribe(subId_);
    }

private:
    void onMarketData(const Event& event) {
        // Try to cast to QuoteEvent
        if (const auto* quote = dynamic_cast<const QuoteEvent*>(&event)) {
            std::cout << "[MarketData] Quote: " << quote->getSymbol()
                      << " Bid: $" << std::fixed << std::setprecision(2) 
                      << quote->getBidPrice()
                      << " x " << quote->getBidSize()
                      << " | Ask: $" << quote->getAskPrice()
                      << " x " << quote->getAskSize()
                      << " | Spread: $" << quote->getSpread()
                      << " | Latency: " << event.getAgeInMicroseconds() << "μs\n";
        }
        // Try to cast to TradeEvent
        else if (const auto* trade = dynamic_cast<const TradeEvent*>(&event)) {
            std::cout << "[MarketData] Trade: " << trade->getSymbol()
                      << " Price: $" << std::fixed << std::setprecision(2)
                      << trade->getPrice()
                      << " Size: " << trade->getSize()
                      << " | Latency: " << event.getAgeInMicroseconds() << "μs\n";
        }
    }

    uint64_t subId_;
};

/**
 * OrderManager: Tracks order lifecycle and fill events
 */
class OrderManager {
public:
    OrderManager() {
        orderSubId_ = EventBus::getInstance().subscribe(
            EventType::ORDER,
            [this](const Event& event) { this->onOrderEvent(event); }
        );
        
        fillSubId_ = EventBus::getInstance().subscribe(
            EventType::FILL,
            [this](const Event& event) { this->onFillEvent(event); }
        );
        
        std::cout << "[OrderManager] Initialized\n";
    }
    
    ~OrderManager() {
        EventBus::getInstance().unsubscribe(orderSubId_);
        EventBus::getInstance().unsubscribe(fillSubId_);
    }

private:
    void onOrderEvent(const Event& event) {
        const auto* order = dynamic_cast<const OrderEvent*>(&event);
        if (!order) return;
        
        std::cout << "[OrderManager] Order " << order->getOrderId()
                  << " | " << order->getSymbol()
                  << " | " << (order->getSide() == Side::BUY ? "BUY" : "SELL")
                  << " | Status: ";
        
        switch (order->getStatus()) {
            case OrderStatus::PENDING_NEW:
                std::cout << "PENDING_NEW";
                break;
            case OrderStatus::NEW:
                std::cout << "NEW (Accepted)";
                break;
            case OrderStatus::PARTIALLY_FILLED:
                std::cout << "PARTIALLY_FILLED (" 
                          << order->getFilledQuantity() << "/" 
                          << order->getQuantity() << ")";
                break;
            case OrderStatus::FILLED:
                std::cout << "FILLED";
                break;
            case OrderStatus::CANCELLED:
                std::cout << "CANCELLED";
                break;
            case OrderStatus::REJECTED:
                std::cout << "REJECTED: " << order->getRejectReason();
                break;
            default:
                std::cout << "UNKNOWN";
        }
        
        std::cout << " | Latency: " << event.getAgeInMicroseconds() << "μs\n";
    }
    
    void onFillEvent(const Event& event) {
        const auto* fill = dynamic_cast<const FillEvent*>(&event);
        if (!fill) return;
        
        std::cout << "[OrderManager] Fill for Order " << fill->getOrderId()
                  << " | " << fill->getSymbol()
                  << " | " << (fill->getSide() == Side::BUY ? "BOUGHT" : "SOLD")
                  << " " << fill->getFillQuantity()
                  << " @ $" << std::fixed << std::setprecision(2) 
                  << fill->getFillPrice()
                  << " | Latency: " << event.getAgeInMicroseconds() << "μs\n";
    }

    uint64_t orderSubId_;
    uint64_t fillSubId_;
};

/**
 * SimpleStrategy: Example strategy that subscribes to market data
 */
class SimpleStrategy {
public:
    SimpleStrategy() {
        subId_ = EventBus::getInstance().subscribe(
            EventType::MARKET_DATA,
            [this](const Event& event) { this->onMarketData(event); }
        );
        
        std::cout << "[Strategy] Initialized\n";
    }
    
    ~SimpleStrategy() {
        EventBus::getInstance().unsubscribe(subId_);
    }

private:
    void onMarketData(const Event& event) {
        // Strategy logic would go here
    }

    uint64_t subId_;
};

/**
 * Demo application showing event system usage
 */
int main() {
    std::cout << "=== Trading Engine Event System Demo ===\n\n";
    
    // Initialize components - they auto-subscribe to events
    MarketDataHandler mdHandler;
    OrderManager orderManager;
    SimpleStrategy strategy;
    
    std::cout << "\n--- Simulating Market Data ---\n";
    
    // Simulate incoming market data
    QuoteEvent quote1("AAPL", 150.25, 150.27, 100, 200);
    EventBus::getInstance().publish(quote1);
    
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    
    QuoteEvent quote2("TSLA", 250.50, 250.55, 300, 150);
    EventBus::getInstance().publish(quote2);
    
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    
    TradeEvent trade1("AAPL", 150.26, 500);
    EventBus::getInstance().publish(trade1);
    
    std::cout << "\n--- Simulating Order Flow ---\n";
    
    // Simulate order lifecycle
    OrderEvent order1("ORD001", "AAPL", Side::BUY, OrderType::LIMIT, 
                      OrderStatus::PENDING_NEW, 150.25, 100);
    EventBus::getInstance().publish(order1);
    
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    
    OrderEvent order2("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::NEW, 150.25, 100);
    EventBus::getInstance().publish(order2);
    
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    // Simulate partial fill
    FillEvent fill1("ORD001", "AAPL", Side::BUY, 150.25, 50);
    EventBus::getInstance().publish(fill1);
    
    OrderEvent order3("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::PARTIALLY_FILLED, 150.25, 100, 50);
    EventBus::getInstance().publish(order3);
    
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    // Complete fill
    FillEvent fill2("ORD001", "AAPL", Side::BUY, 150.26, 50);
    EventBus::getInstance().publish(fill2);
    
    OrderEvent order4("ORD001", "AAPL", Side::BUY, OrderType::LIMIT,
                      OrderStatus::FILLED, 150.25, 100, 100);
    EventBus::getInstance().publish(order4);
    
    std::cout << "\n--- System Events ---\n";
    
    // System events
    SystemEvent startup(SystemEventType::TRADING_START, "Trading session started");
    EventBus::getInstance().subscribe(
        EventType::SYSTEM,
        [](const Event& event) {
            const auto* sysEvent = dynamic_cast<const SystemEvent*>(&event);
            if (sysEvent) {
                std::cout << "[System] " << sysEvent->getMessage() << "\n";
            }
        }
    );
    EventBus::getInstance().publish(startup);
    
    // Timer event example
    std::cout << "\n--- Timer Events ---\n";
    EventBus::getInstance().subscribe(
        EventType::TIMER,
        [](const Event& event) {
            const auto* timer = dynamic_cast<const TimerEvent*>(&event);
            if (timer) {
                std::cout << "[Timer] '" << timer->getName() << "' fired\n";
                if (timer->hasCallback()) {
                    timer->execute();
                }
            }
        }
    );
    
    TimerEvent heartbeat("heartbeat", []() {
        std::cout << "  -> Heartbeat callback executed\n";
    });
    EventBus::getInstance().publish(heartbeat);
    
    std::cout << "\n--- Statistics ---\n";
    std::cout << "Total events processed: " 
              << EventBus::getInstance().getEventCount() << "\n";
    std::cout << "Queue size: " 
              << EventBus::getInstance().getQueueSize() << "\n";
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
