#pragma once

#include <string>
#include <vector>

namespace engine {

/**
 * Trade data point from historical data
 */
struct TradeData {
    int64_t timestamp;      // Unix timestamp in milliseconds
    std::string symbol;
    double price;
    int64_t volume;

    TradeData(int64_t ts, const std::string& sym, double p, int64_t vol)
        : timestamp(ts), symbol(sym), price(p), volume(vol) {}
};

/**
 * HistoricalDataLoader: Loads historical market data from various sources.
 * 
 * Supports:
 * - CSV files with trade data
 * - Future: databases, APIs, binary formats
 */
class HistoricalDataLoader {
public:
    /**
     * Load trade data from CSV file
     * Expected format: timestamp,symbol,price,volume
     * timestamp can be Unix milliseconds or ISO format
     */
    static std::vector<TradeData> loadFromCSV(const std::string& filename);

    /**
     * Filter data by symbol
     */
    static std::vector<TradeData> filterBySymbol(
        const std::vector<TradeData>& data,
        const std::string& symbol);

    /**
     * Filter data by time range
     */
    static std::vector<TradeData> filterByTimeRange(
        const std::vector<TradeData>& data,
        int64_t startTime,
        int64_t endTime);

    /**
     * Sort data by timestamp (ascending)
     */
    static void sortByTimestamp(std::vector<TradeData>& data);

private:
    static int64_t parseTimestamp(const std::string& timestampStr);
    static std::string trim(const std::string& str);
};

} // namespace engine
