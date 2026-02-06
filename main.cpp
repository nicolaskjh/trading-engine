#include "event/EventBus.h"
#include "logger/Logger.h"
#include "data/BookManager.h"
#include "data/MarketDataHandler.h"
#include "order/OrderLogger.h"
#include "order/OrderManager.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

using namespace engine;

// Global flag for graceful shutdown
std::atomic<bool> running{true};

/**
 * Signal handler for graceful shutdown
 */
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        Logger::warning(LogComponent::ENGINE, "Shutdown signal received, stopping engine...");
        running = false;
    }
}

/**
 * Trading Engine entry point
 */
int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::init(LogLevel::INFO);
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "========================================\n";
    std::cout << "   Trading Engine v1.0\n";
    std::cout << "========================================\n\n";
    
    try {
        // Initialize core components
        OrderManager orderManager;
        BookManager bookManager;
        MarketDataHandler marketDataHandler;
        OrderLogger orderLogger;
        
        Logger::info(LogComponent::ENGINE, "Initialized");
        Logger::info(LogComponent::ENGINE, "Ready to trade");
        Logger::info(LogComponent::ENGINE, "Press Ctrl+C to shutdown");
        
        // Main event loop
        while (running) {
            // Process any queued events
            EventBus::getInstance().processQueue(10);
            
            // In a real system, this would:
            // - Process incoming market data from exchanges
            // - Execute strategy logic based on market conditions
            // - Send orders to exchanges
            // - Handle fills and position updates
            // - Monitor risk limits
            
            // Sleep briefly to avoid busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        Logger::info(LogComponent::ENGINE, "Shutting down...");
        Logger::info(LogComponent::ENGINE, "Final stats:");
        Logger::info(LogComponent::ENGINE, "Total events processed: " + 
                     std::to_string(EventBus::getInstance().getEventCount()));
        Logger::info(LogComponent::ENGINE, "Active orders: " + 
                     std::to_string(orderManager.getActiveOrderCount()));
        Logger::info(LogComponent::ENGINE, "Tracked symbols: " + 
                     std::to_string(bookManager.getBookCount()));
        Logger::info(LogComponent::ENGINE, "Shutdown complete");
        
        Logger::shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        Logger::critical(LogComponent::ENGINE, "Fatal error: " + std::string(e.what()));
        Logger::shutdown();
        return 1;
    }
}
