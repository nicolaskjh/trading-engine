/**
 * SMA Strategy Backtest
 * 
 * Backtests a Simple Moving Average crossover strategy on historical data.
 * The strategy goes long when the fast SMA crosses above the slow SMA (golden cross)
 * and short when it crosses below (death cross).
 */

#include "backtesting/Backtester.h"
#include "config/Config.h"
#include "risk/Portfolio.h"
#include "strategy/SMAStrategy.h"

#include <iomanip>
#include <iostream>
#include <memory>

using namespace engine;

int main() {
    std::cout << "=== SMA Strategy Backtest ===" << std::endl;
    std::cout << std::endl;
    
    // Load configuration
    try {
        Config::loadFromFile("config.ini");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return 1;
    }
    
    // Get backtest parameters from config
    double initialCapital = Config::getDouble("backtest.initial_capital", 1000000.0);
    std::string dataFile = Config::getString("backtest.data_file", "data/historical_trades.csv");
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Initial Capital: $" << std::fixed << std::setprecision(2) << initialCapital << std::endl;
    std::cout << "  Data File: " << dataFile << std::endl;
    std::cout << "  SMA Fast Period: " << Config::getInt("strategy.sma.fast_period", 20) << std::endl;
    std::cout << "  SMA Slow Period: " << Config::getInt("strategy.sma.slow_period", 50) << std::endl;
    std::cout << "  Position Size: " << Config::getInt("strategy.sma.position_size", 100) << std::endl;
    std::cout << std::endl;
    
    // Create backtester
    Backtester backtester(initialCapital);
    
    // Get portfolio for strategy
    auto portfolio = backtester.getPortfolio();
    
    // Create SMA strategy for AAPL
    auto strategy = std::make_shared<SMAStrategy>(
        "SMA_AAPL",
        std::shared_ptr<Portfolio>(portfolio, [](Portfolio*){}),  // Non-owning shared_ptr
        "AAPL"
    );
    
    backtester.addStrategy(strategy);
    
    // Load historical data
    try {
        std::cout << "Loading historical data..." << std::endl;
        backtester.loadData(dataFile);
        std::cout << "Data loaded successfully" << std::endl;
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load data: " << e.what() << std::endl;
        return 1;
    }
    
    // Run backtest
    std::cout << "Running backtest..." << std::endl;
    BacktestResults results;
    
    try {
        results = backtester.run();
        std::cout << "Backtest completed successfully" << std::endl;
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Backtest failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Display results
    std::cout << results.toString() << std::endl;
    
    // Display portfolio state
    std::cout << "Final Portfolio State:" << std::endl;
    std::cout << "  Cash: $" << std::fixed << std::setprecision(2) << portfolio->getCash() << std::endl;
    std::cout << "  Realized P&L: $" << std::fixed << std::setprecision(2) << portfolio->getRealizedPnL() << std::endl;
    std::cout << std::endl;
    
    // Display open positions
    auto positions = portfolio->getOrderManager()->getAllPositions();
    if (!positions.empty()) {
        std::cout << "Open Positions:" << std::endl;
        for (const auto& position : positions) {
            std::cout << "  " << position->getSymbol() 
                      << ": " << position->getQuantity() 
                      << " @ $" << std::fixed << std::setprecision(2) << position->getAveragePrice()
                      << " (Realized P&L: $" << std::fixed << std::setprecision(2) << position->getRealizedPnL() << ")"
                      << std::endl;
        }
    } else {
        std::cout << "No open positions" << std::endl;
    }
    
    return 0;
}
