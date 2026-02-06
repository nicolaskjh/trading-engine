#pragma once

#include <string>
#include <vector>

namespace engine {

/**
 * Portfolio snapshot at a specific point in time
 */
struct PortfolioSnapshot {
    int64_t timestamp;
    double portfolioValue;
    double cash;
    double realizedPnL;
    double unrealizedPnL;

    PortfolioSnapshot(int64_t ts, double value, double c, double realized, double unrealized)
        : timestamp(ts), portfolioValue(value), cash(c), 
          realizedPnL(realized), unrealizedPnL(unrealized) {}
};

/**
 * Backtest results with comprehensive performance metrics
 */
struct BacktestResults {
    // Basic metrics
    double totalReturn;              // Percentage return
    double totalReturnDollars;       // Dollar return
    int totalTrades;
    int winningTrades;
    int losingTrades;
    
    // Risk metrics
    double sharpeRatio;              // Annualized Sharpe ratio
    double maxDrawdown;              // Maximum drawdown percentage
    double maxDrawdownDollars;       // Maximum drawdown in dollars
    
    // Trade statistics
    double winRate;                  // Percentage of winning trades
    double averageWin;
    double averageLoss;
    double profitFactor;             // Gross profit / gross loss
    double largestWin;
    double largestLoss;
    
    // Time metrics
    int64_t startTime;
    int64_t endTime;
    double durationDays;
    
    BacktestResults();
    std::string toString() const;
};

/**
 * PerformanceMetrics: Calculate trading strategy performance metrics.
 * 
 * Provides comprehensive analysis including:
 * - Returns and profitability
 * - Risk metrics (Sharpe, drawdown)
 * - Trade statistics
 */
class PerformanceMetrics {
public:
    /**
     * Calculate all performance metrics from portfolio snapshots
     */
    static BacktestResults calculate(
        const std::vector<PortfolioSnapshot>& snapshots,
        double initialCapital,
        double riskFreeRate = 0.02);  // 2% annual risk-free rate

    /**
     * Calculate total return percentage
     */
    static double calculateTotalReturn(double initialValue, double finalValue);

    /**
     * Calculate Sharpe ratio (annualized)
     */
    static double calculateSharpeRatio(
        const std::vector<double>& returns,
        double riskFreeRate = 0.02);

    /**
     * Calculate maximum drawdown
     */
    static double calculateMaxDrawdown(const std::vector<double>& portfolioValues);

    /**
     * Calculate win rate from trades
     */
    static double calculateWinRate(int winningTrades, int totalTrades);

private:
    static std::vector<double> calculateReturns(const std::vector<double>& values);
    static double calculateMean(const std::vector<double>& values);
    static double calculateStdDev(const std::vector<double>& values, double mean);
};

} // namespace engine
