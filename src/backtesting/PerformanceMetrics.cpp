#include "backtesting/PerformanceMetrics.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace engine {

BacktestResults::BacktestResults()
    : totalReturn(0.0)
    , totalReturnDollars(0.0)
    , totalTrades(0)
    , winningTrades(0)
    , losingTrades(0)
    , sharpeRatio(0.0)
    , maxDrawdown(0.0)
    , maxDrawdownDollars(0.0)
    , winRate(0.0)
    , averageWin(0.0)
    , averageLoss(0.0)
    , profitFactor(0.0)
    , largestWin(0.0)
    , largestLoss(0.0)
    , startTime(0)
    , endTime(0)
    , durationDays(0.0)
{}

std::string BacktestResults::toString() const {
    std::stringstream ss;
    ss << "\n=== Backtest Results ===\n";
    ss << "Total Return: " << (totalReturn * 100.0) << "%\n";
    ss << "Total Return ($): $" << totalReturnDollars << "\n";
    ss << "Sharpe Ratio: " << sharpeRatio << "\n";
    ss << "Max Drawdown: " << (maxDrawdown * 100.0) << "%\n";
    ss << "Max Drawdown ($): $" << maxDrawdownDollars << "\n";
    ss << "\nTrade Statistics:\n";
    ss << "Total Trades: " << totalTrades << "\n";
    ss << "Winning Trades: " << winningTrades << "\n";
    ss << "Losing Trades: " << losingTrades << "\n";
    ss << "Win Rate: " << (winRate * 100.0) << "%\n";
    ss << "Average Win: $" << averageWin << "\n";
    ss << "Average Loss: $" << averageLoss << "\n";
    ss << "Profit Factor: " << profitFactor << "\n";
    ss << "Largest Win: $" << largestWin << "\n";
    ss << "Largest Loss: $" << largestLoss << "\n";
    ss << "\nDuration: " << durationDays << " days\n";
    return ss.str();
}

BacktestResults PerformanceMetrics::calculate(
    const std::vector<PortfolioSnapshot>& snapshots,
    double initialCapital,
    double riskFreeRate) {
    
    BacktestResults results;
    
    if (snapshots.empty()) {
        return results;
    }
    
    // Extract portfolio values
    std::vector<double> portfolioValues;
    portfolioValues.reserve(snapshots.size());
    for (const auto& snapshot : snapshots) {
        portfolioValues.push_back(snapshot.portfolioValue);
    }
    
    // Basic metrics
    double finalValue = snapshots.back().portfolioValue;
    results.totalReturn = calculateTotalReturn(initialCapital, finalValue);
    results.totalReturnDollars = finalValue - initialCapital;
    
    // Time metrics
    results.startTime = snapshots.front().timestamp;
    results.endTime = snapshots.back().timestamp;
    results.durationDays = (results.endTime - results.startTime) / (1000.0 * 86400.0);
    
    // Risk metrics
    std::vector<double> returns = calculateReturns(portfolioValues);
    results.sharpeRatio = calculateSharpeRatio(returns, riskFreeRate);
    results.maxDrawdown = calculateMaxDrawdown(portfolioValues);
    
    // Calculate max drawdown in dollars
    double peak = portfolioValues[0];
    double maxDD = 0.0;
    for (double value : portfolioValues) {
        if (value > peak) peak = value;
        double dd = peak - value;
        if (dd > maxDD) maxDD = dd;
    }
    results.maxDrawdownDollars = maxDD;
    
    // Trade statistics (extract from snapshots with realized P&L changes)
    double totalWinAmount = 0.0;
    double totalLossAmount = 0.0;
    double previousRealizedPnL = 0.0;
    
    for (const auto& snapshot : snapshots) {
        double realizedChange = snapshot.realizedPnL - previousRealizedPnL;
        
        if (std::abs(realizedChange) > 0.01) {  // New trade closed
            results.totalTrades++;
            
            if (realizedChange > 0) {
                results.winningTrades++;
                totalWinAmount += realizedChange;
                if (realizedChange > results.largestWin) {
                    results.largestWin = realizedChange;
                }
            } else {
                results.losingTrades++;
                totalLossAmount += std::abs(realizedChange);
                if (std::abs(realizedChange) > std::abs(results.largestLoss)) {
                    results.largestLoss = realizedChange;
                }
            }
        }
        
        previousRealizedPnL = snapshot.realizedPnL;
    }
    
    // Calculate derived metrics
    results.winRate = calculateWinRate(results.winningTrades, results.totalTrades);
    results.averageWin = (results.winningTrades > 0) ? totalWinAmount / results.winningTrades : 0.0;
    results.averageLoss = (results.losingTrades > 0) ? totalLossAmount / results.losingTrades : 0.0;
    results.profitFactor = (totalLossAmount > 0) ? totalWinAmount / totalLossAmount : 0.0;
    
    return results;
}

double PerformanceMetrics::calculateTotalReturn(double initialValue, double finalValue) {
    if (initialValue == 0.0) return 0.0;
    return (finalValue - initialValue) / initialValue;
}

double PerformanceMetrics::calculateSharpeRatio(
    const std::vector<double>& returns,
    double riskFreeRate) {
    
    if (returns.empty()) return 0.0;
    
    // Annualized daily risk-free rate
    double dailyRiskFreeRate = std::pow(1.0 + riskFreeRate, 1.0 / 252.0) - 1.0;
    
    // Calculate excess returns
    std::vector<double> excessReturns;
    excessReturns.reserve(returns.size());
    for (double ret : returns) {
        excessReturns.push_back(ret - dailyRiskFreeRate);
    }
    
    double mean = calculateMean(excessReturns);
    double stdDev = calculateStdDev(excessReturns, mean);
    
    if (stdDev == 0.0) return 0.0;
    
    // Annualize (assume daily data, 252 trading days)
    return (mean / stdDev) * std::sqrt(252.0);
}

double PerformanceMetrics::calculateMaxDrawdown(const std::vector<double>& portfolioValues) {
    if (portfolioValues.empty()) return 0.0;
    
    double maxDrawdown = 0.0;
    double peak = portfolioValues[0];
    
    for (double value : portfolioValues) {
        if (value > peak) {
            peak = value;
        }
        
        double drawdown = (peak - value) / peak;
        if (drawdown > maxDrawdown) {
            maxDrawdown = drawdown;
        }
    }
    
    return maxDrawdown;
}

double PerformanceMetrics::calculateWinRate(int winningTrades, int totalTrades) {
    if (totalTrades == 0) return 0.0;
    return static_cast<double>(winningTrades) / totalTrades;
}

std::vector<double> PerformanceMetrics::calculateReturns(const std::vector<double>& values) {
    std::vector<double> returns;
    if (values.size() < 2) return returns;
    
    returns.reserve(values.size() - 1);
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i - 1] != 0.0) {
            returns.push_back((values[i] - values[i - 1]) / values[i - 1]);
        }
    }
    
    return returns;
}

double PerformanceMetrics::calculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    
    return sum / values.size();
}

double PerformanceMetrics::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() < 2) return 0.0;
    
    double sumSquaredDiff = 0.0;
    for (double value : values) {
        double diff = value - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return std::sqrt(sumSquaredDiff / (values.size() - 1));
}

} // namespace engine
