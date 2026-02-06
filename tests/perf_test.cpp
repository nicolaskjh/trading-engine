/**
 * Performance Test
 * 
 * Tests the trading engine's performance under load:
 * - Tick-to-trade latency (market data → strategy → order → fill)
 * - Order submission latency
 * - Event processing latency  
 * - Fill execution latency
 * - Jitter (consecutive latency variation)
 * - Throughput (orders/second)
 */

#include "config/Config.h"
#include "event/EventBus.h"
#include "exchange/SimulatedExchange.h"
#include "logger/Logger.h"
#include "perf/LatencyStats.h"
#include "risk/Portfolio.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace engine;

class PerformanceTester {
public:
    PerformanceTester() {
        // Disable logging during performance tests
        Logger::init(LogLevel::WARNING);
        
        // Create portfolio with very large capital for stress testing
        portfolio_ = std::make_shared<Portfolio>(1000000000.0); // $1B capital
        
        // Configure exchange for instant fills (no latency simulation)
        SimulatedExchange::Config exchangeConfig;
        exchangeConfig.instantFills = true;
        exchangeConfig.rejectionRate = 0.0;
        exchangeConfig.partialFillRate = 0.0;
        exchangeConfig.slippageBps = 0.0;
        
        exchange_ = std::make_unique<SimulatedExchange>(exchangeConfig);
        exchange_->start();
        
        // Set market prices
        exchange_->setMarketPrice("AAPL", 250.0);
        exchange_->setMarketPrice("MSFT", 400.0);
        exchange_->setMarketPrice("GOOGL", 150.0);
        
        // Subscribe to fill events to measure latency
        fillSubId_ = EventBus::getInstance().subscribe(
            EventType::FILL,
            [this](const Event& event) { this->onFillEvent(event); }
        );
    }
    
    ~PerformanceTester() {
        EventBus::getInstance().unsubscribe(fillSubId_);
        exchange_->stop();
    }
    
    void runTest(size_t numOrders) {
        std::cout << "\n=== Running Performance Test with " << numOrders << " orders ===" << std::endl;
        
        // For very large tests, sample to reduce memory pressure
        bool useSampling = numOrders > 200000;
        size_t sampleRate = useSampling ? (numOrders / 100000) : 1; // Sample ~100K for large tests
        
        std::cout << "Sampling: " << (useSampling ? ("1 in " + std::to_string(sampleRate)) : "all orders") << std::endl;
        
        // Clear and pre-allocate memory to avoid dynamic growth
        orderLatencies_.clear();
        fillLatencies_.clear();
        tickToTradeLatencies_.clear();
        strategyLatencies_.clear();
        
        // Pre-reserve space in latency vectors
        size_t reserveSize = useSampling ? (numOrders / sampleRate + 1000) : numOrders;
        orderLatencies_.reserve(reserveSize);
        fillLatencies_.reserve(reserveSize);
        tickToTradeLatencies_.reserve(reserveSize);
        strategyLatencies_.reserve(numOrders);
        
        // Clear maps and pre-reserve capacity to avoid rehashing
        orderTimestamps_.clear();
        tickTimestamps_.clear();
        orderTimestamps_.reserve(reserveSize);
        tickTimestamps_.reserve(reserveSize);
        
        std::unordered_map<std::string, double> marketPrices = {
            {"AAPL", 250.0},
            {"MSFT", 400.0},
            {"GOOGL", 150.0}
        };
        
        // Pre-generate order IDs to avoid string concatenation in hot path
        std::vector<std::string> orderIds;
        orderIds.reserve(numOrders);
        for (size_t i = 0; i < numOrders; ++i) {
            orderIds.push_back("PERF_" + std::to_string(i));
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Simulate tick-to-trade: Market data arrives → Strategy decides → Order submitted → Fill
        for (size_t i = 0; i < numOrders; ++i) {
            const std::string& orderId = orderIds[i];
            std::string symbol = (i % 3 == 0) ? "AAPL" : (i % 3 == 1) ? "MSFT" : "GOOGL";
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            
            // Only store timestamps for sampled orders to reduce memory
            bool shouldSample = !useSampling || (i % sampleRate == 0);
            
            // 1. Market data tick arrives
            auto tickArrival = std::chrono::high_resolution_clock::now();
            if (shouldSample) {
                tickTimestamps_[orderId] = tickArrival;
            }
            
            // 2. Strategy processes tick and makes decision (simulate ~100ns of logic)
            double price = marketPrices[symbol];
            bool shouldTrade = (price > 0); // Simple decision logic
            
            auto afterStrategy = std::chrono::high_resolution_clock::now();
            auto strategyLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                afterStrategy - tickArrival
            ).count();
            strategyLatencies_.addSample(strategyLatency);
            
            // 3. Submit order (if strategy decided to trade)
            if (shouldTrade) {
                auto orderStart = std::chrono::high_resolution_clock::now();
                if (shouldSample) {
                    orderTimestamps_[orderId] = orderStart;
                }
                
                portfolio_->submitOrder(
                    orderId,
                    symbol,
                    side,
                    OrderType::MARKET,
                    price,
                    100,  // 100 shares
                    marketPrices
                );
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime
        ).count();
        
        // Wait for all async processing to complete (scale with order count)
        size_t waitMs = 100 + (numOrders / 10000); // Base 100ms + 1ms per 10K orders
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
        
        std::cout << "\nProcessing complete. Collecting statistics..." << std::endl;
        std::cout << "Orders submitted: " << numOrders << std::endl;
        std::cout << "Fills received: " << orderLatencies_.getCount() << std::endl;
        
        // Calculate statistics
        orderLatencies_.calculate();
        fillLatencies_.calculate();
        tickToTradeLatencies_.calculate();
        strategyLatencies_.calculate();
        
        // Calculate throughput
        double durationSeconds = durationUs / 1000000.0;
        double throughput = numOrders / durationSeconds;
        
        // Calculate jitter (standard deviation of consecutive latency differences)
        double orderJitter = calculateJitter(orderLatencies_);
        double tickToTradeJitter = calculateJitter(tickToTradeLatencies_);
        
        // Display results
        std::cout << "\n=== Performance Results ===" << std::endl;
        
        std::cout << "\nTick-to-Trade Latency (Market Data → Fill):" << std::endl;
        std::cout << tickToTradeLatencies_.report() << std::endl;
        std::cout << "  Jitter: " << std::fixed << std::setprecision(2) << tickToTradeJitter << " μs" << std::endl;
        
        std::cout << "\nStrategy Decision Latency:" << std::endl;
        std::cout << "  Mean:    " << std::fixed << std::setprecision(2) 
                  << (strategyLatencies_.getMean() / 1000.0) << " μs" << std::endl;
        std::cout << "  Median:  " << std::fixed << std::setprecision(2) 
                  << (strategyLatencies_.getMedian() / 1000.0) << " μs" << std::endl;
        
        std::cout << "\nOrder Submission Latency (Order → Fill):" << std::endl;
        std::cout << orderLatencies_.report() << std::endl;
        std::cout << "  Jitter: " << std::fixed << std::setprecision(2) << orderJitter << " μs" << std::endl;
        
        std::cout << "\nFill Event Processing Latency:" << std::endl;
        std::cout << fillLatencies_.report() << std::endl;
        
        std::cout << "\n=== Throughput ===" << std::endl;
        std::cout << "Total Time: " << std::fixed << std::setprecision(2) 
                  << durationSeconds << " seconds" << std::endl;
        std::cout << "Orders/Second: " << std::fixed << std::setprecision(0) 
                  << throughput << std::endl;
        std::cout << "Average Order Latency: " << std::fixed << std::setprecision(2)
                  << (durationUs / static_cast<double>(numOrders)) << " μs" << std::endl;
    }
    
private:
    void onFillEvent(const Event& event) {
        const auto* fillEvent = dynamic_cast<const FillEvent*>(&event);
        if (!fillEvent) return;
        
        // Record fill latency (time from event creation)
        fillLatencies_.addSample(event.getAgeInMicroseconds());
        
        // Calculate end-to-end latency (from order submission to fill)
        auto it = orderTimestamps_.find(fillEvent->getOrderId());
        if (it != orderTimestamps_.end()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto e2eLatency = std::chrono::duration_cast<std::chrono::microseconds>(
                now - it->second
            ).count();
            orderLatencies_.addSample(e2eLatency);
        }
        
        // Calculate tick-to-trade latency (from market data arrival to fill)
        auto tickIt = tickTimestamps_.find(fillEvent->getOrderId());
        if (tickIt != tickTimestamps_.end()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto tickToTradeLatency = std::chrono::duration_cast<std::chrono::microseconds>(
                now - tickIt->second
            ).count();
            tickToTradeLatencies_.addSample(tickToTradeLatency);
        }
    }
    
    // Calculate jitter: standard deviation of consecutive latency differences
    double calculateJitter(const LatencyStats& stats) {
        if (stats.getCount() < 2) return 0.0;
        
        // Get all samples (we'd need to expose this in LatencyStats, for now use stddev as proxy)
        // In a real implementation, we'd calculate actual inter-sample variation
        // For now, use stddev as an approximation of jitter
        return stats.getStdDev();
    }
    
    std::shared_ptr<Portfolio> portfolio_;
    std::unique_ptr<SimulatedExchange> exchange_;
    
    LatencyStats orderLatencies_;
    LatencyStats fillLatencies_;
    LatencyStats tickToTradeLatencies_;
    LatencyStats strategyLatencies_;
    
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> orderTimestamps_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> tickTimestamps_;
    
    uint64_t fillSubId_;
};

int main() {
    std::cout << "=== Trading Engine Performance Test ===" << std::endl;
    
    // Load minimal config
    Config::loadFromFile("config.ini");
    
    PerformanceTester tester;
    
    // Run baseline tests
    std::cout << "\n========================================" << std::endl;
    std::cout << "BASELINE TESTS" << std::endl;
    std::cout << "========================================" << std::endl;
    tester.runTest(1000);      // 1K orders
    tester.runTest(10000);     // 10K orders
    tester.runTest(100000);    // 100K orders
    
    // Run stress tests
    std::cout << "\n========================================" << std::endl;
    std::cout << "STRESS TESTS" << std::endl;
    std::cout << "========================================" << std::endl;
    tester.runTest(500000);    // 500K orders
    tester.runTest(1000000);   // 1M orders
    tester.runTest(5000000);   // 5M orders
    
    std::cout << "\n=== Performance Test Complete ===" << std::endl;
    
    return 0;
}
