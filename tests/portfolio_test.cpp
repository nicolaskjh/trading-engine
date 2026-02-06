#include "event/EventBus.h"
#include "order/Order.h"
#include "risk/Portfolio.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace engine;

void test_portfolio_initialization() {
    std::cout << "Testing portfolio initialization..." << std::endl;
    
    Portfolio portfolio(100000.0);  // $100k initial capital
    
    assert(portfolio.getInitialCapital() == 100000.0);
    assert(portfolio.getCash() == 100000.0);
    assert(portfolio.getRealizedPnL() == 0.0);
    
    std::cout << "  ✓ Portfolio initialization passed" << std::endl;
}

void test_portfolio_value() {
    std::cout << "Testing portfolio value calculation..." << std::endl;
    
    Portfolio portfolio(100000.0);
    
    // Submit and fill a buy order
    std::unordered_map<std::string, double> prices = {{"AAPL", 150.0}};
    
    assert(portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT, 
                                 150.0, 100, prices));
    
    // Simulate fill event
    FillEvent fill("order1", "AAPL", Side::BUY, 100, 150.0);
    EventBus::getInstance().publish(fill);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Portfolio value should be cash + unrealized P&L
    double portfolioValue = portfolio.getPortfolioValue(prices);
    assert(portfolioValue == 100000.0);  // No P&L at same price
    
    // Price increases
    prices["AAPL"] = 160.0;
    portfolioValue = portfolio.getPortfolioValue(prices);
    assert(portfolioValue == 101000.0);  // Cash (100k) + unrealized P&L (1k)
    
    std::cout << "  ✓ Portfolio value calculation passed" << std::endl;
}

void test_exposure_calculation() {
    std::cout << "Testing exposure calculation..." << std::endl;
    
    Portfolio portfolio(1000000.0);  // $1M initial capital
    
    std::unordered_map<std::string, double> prices = {
        {"AAPL", 150.0},
        {"GOOGL", 2800.0}
    };
    
    // Buy AAPL
    assert(portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                 150.0, 100, prices));
    FillEvent fill1("order1", "AAPL", Side::BUY, 100, 150.0);
    EventBus::getInstance().publish(fill1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Sell GOOGL (short)
    assert(portfolio.submitOrder("order2", "GOOGL", Side::SELL, OrderType::LIMIT,
                                 2800.0, 10, prices));
    FillEvent fill2("order2", "GOOGL", Side::SELL, 10, 2800.0);
    EventBus::getInstance().publish(fill2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Gross exposure: |100 * 150| + |10 * 2800| = 15,000 + 28,000 = 43,000
    double grossExposure = portfolio.getGrossExposure(prices);
    assert(grossExposure == 43000.0);
    
    // Net exposure: (100 * 150) + (-10 * 2800) = 15,000 - 28,000 = -13,000
    double netExposure = portfolio.getNetExposure(prices);
    assert(netExposure == -13000.0);
    
    std::cout << "  ✓ Exposure calculation passed" << std::endl;
}

void test_position_size_limit() {
    std::cout << "Testing position size limit..." << std::endl;
    
    Portfolio portfolio(1000000.0);
    portfolio.setMaxPositionSize(20000.0);  // Max $20k per position
    
    std::unordered_map<std::string, double> prices = {{"AAPL", 150.0}};
    
    // Try to buy $22,500 worth (150 shares @ $150)
    bool accepted = portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                         150.0, 150, prices);
    assert(!accepted);  // Should be rejected
    
    // Try to buy $15,000 worth (100 shares @ $150)
    accepted = portfolio.submitOrder("order2", "AAPL", Side::BUY, OrderType::LIMIT,
                                    150.0, 100, prices);
    assert(accepted);  // Should be accepted
    
    std::cout << "  ✓ Position size limit passed" << std::endl;
}

void test_portfolio_exposure_limit() {
    std::cout << "Testing portfolio exposure limit..." << std::endl;
    
    Portfolio portfolio(1000000.0);
    portfolio.setMaxPositionSize(100000.0);  // Max $100k per position
    portfolio.setMaxPortfolioExposure(50000.0);  // Max $50k total exposure
    
    std::unordered_map<std::string, double> prices = {
        {"AAPL", 150.0},
        {"GOOGL", 2800.0}
    };
    
    // Buy $30k AAPL (200 shares @ $150)
    bool accepted = portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                         150.0, 200, prices);
    assert(accepted);
    FillEvent fill1("order1", "AAPL", Side::BUY, 200, 150.0);
    EventBus::getInstance().publish(fill1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Try to buy $28k GOOGL (10 shares @ $2800) - would exceed portfolio limit
    accepted = portfolio.submitOrder("order2", "GOOGL", Side::BUY, OrderType::LIMIT,
                                    2800.0, 10, prices);
    assert(!accepted);  // Should be rejected (30k + 28k = 58k > 50k limit)
    
    // Try to buy $14k GOOGL (5 shares @ $2800) - within portfolio limit
    accepted = portfolio.submitOrder("order3", "GOOGL", Side::BUY, OrderType::LIMIT,
                                    2800.0, 5, prices);
    assert(accepted);  // Should be accepted (30k + 14k = 44k < 50k limit)
    
    std::cout << "  ✓ Portfolio exposure limit passed" << std::endl;
}

void test_insufficient_cash() {
    std::cout << "Testing insufficient cash check..." << std::endl;
    
    Portfolio portfolio(10000.0);  // Only $10k
    
    std::unordered_map<std::string, double> prices = {{"AAPL", 150.0}};
    
    // Try to buy $15k worth (100 shares @ $150)
    bool accepted = portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                         150.0, 100, prices);
    assert(!accepted);  // Should be rejected (insufficient cash)
    
    // Try to buy $7.5k worth (50 shares @ $150)
    accepted = portfolio.submitOrder("order2", "AAPL", Side::BUY, OrderType::LIMIT,
                                    150.0, 50, prices);
    assert(accepted);  // Should be accepted
    
    std::cout << "  ✓ Insufficient cash check passed" << std::endl;
}

void test_pnl_tracking() {
    std::cout << "Testing P&L tracking..." << std::endl;
    
    Portfolio portfolio(100000.0);
    
    std::unordered_map<std::string, double> prices = {{"AAPL", 150.0}};
    
    // Buy 100 shares @ 150
    assert(portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                 150.0, 100, prices));
    FillEvent fill1("order1", "AAPL", Side::BUY, 100, 150.0);
    EventBus::getInstance().publish(fill1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Unrealized P&L at same price should be 0
    assert(portfolio.getUnrealizedPnL(prices) == 0.0);
    
    // Price increases to 160
    prices["AAPL"] = 160.0;
    assert(portfolio.getUnrealizedPnL(prices) == 1000.0);  // 100 * (160 - 150)
    
    // Sell 50 shares @ 160 (realize half the profit)
    assert(portfolio.submitOrder("order2", "AAPL", Side::SELL, OrderType::LIMIT,
                                 160.0, 50, prices));
    FillEvent fill2("order2", "AAPL", Side::SELL, 50, 160.0);
    EventBus::getInstance().publish(fill2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Realized P&L should be 500 (50 * (160 - 150))
    assert(portfolio.getRealizedPnL() == 500.0);
    
    // Unrealized P&L should be 500 (50 remaining shares * (160 - 150))
    assert(portfolio.getUnrealizedPnL(prices) == 500.0);
    
    // Total P&L should be 1000
    assert(portfolio.getTotalPnL(prices) == 1000.0);
    
    std::cout << "  ✓ P&L tracking passed" << std::endl;
}

void test_portfolio_clear() {
    std::cout << "Testing portfolio clear..." << std::endl;
    
    Portfolio portfolio(100000.0);
    
    std::unordered_map<std::string, double> prices = {{"AAPL", 150.0}};
    
    assert(portfolio.submitOrder("order1", "AAPL", Side::BUY, OrderType::LIMIT,
                                 150.0, 100, prices));
    FillEvent fill("order1", "AAPL", Side::BUY, 100, 150.0);
    EventBus::getInstance().publish(fill);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    portfolio.clear();
    
    assert(portfolio.getCash() == 100000.0);
    assert(portfolio.getRealizedPnL() == 0.0);
    assert(portfolio.getOrderManager()->getActiveOrderCount() == 0);
    
    std::cout << "  ✓ Portfolio clear passed" << std::endl;
}

#ifndef TEST_ALL
int main() {
    std::cout << "\n=== Portfolio Tests ===" << std::endl;
    
    test_portfolio_initialization();
    test_portfolio_value();
    test_exposure_calculation();
    test_position_size_limit();
    test_portfolio_exposure_limit();
    test_insufficient_cash();
    test_pnl_tracking();
    test_portfolio_clear();
    
    std::cout << "\n✓ All portfolio tests passed!\n" << std::endl;
    return 0;
}
#endif
