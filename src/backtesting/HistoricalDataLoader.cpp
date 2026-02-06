#include "backtesting/HistoricalDataLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace engine {

std::vector<TradeData> HistoricalDataLoader::loadFromCSV(const std::string& filename) {
    std::vector<TradeData> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Skip empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Skip header line if it contains "timestamp" or "symbol"
        if (firstLine) {
            firstLine = false;
            std::string lowerLine = line;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
            if (lowerLine.find("timestamp") != std::string::npos || 
                lowerLine.find("symbol") != std::string::npos) {
                continue;
            }
        }
        
        // Parse CSV line: timestamp,symbol,price,volume
        std::stringstream ss(line);
        std::string timestampStr, symbol, priceStr, volumeStr;
        
        if (!std::getline(ss, timestampStr, ',') ||
            !std::getline(ss, symbol, ',') ||
            !std::getline(ss, priceStr, ',') ||
            !std::getline(ss, volumeStr, ',')) {
            throw std::runtime_error("Invalid CSV format at line " + std::to_string(lineNumber));
        }
        
        try {
            int64_t timestamp = parseTimestamp(trim(timestampStr));
            symbol = trim(symbol);
            double price = std::stod(trim(priceStr));
            int64_t volume = std::stoll(trim(volumeStr));
            
            data.emplace_back(timestamp, symbol, price, volume);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
    
    file.close();
    
    // Sort by timestamp
    sortByTimestamp(data);
    
    return data;
}

std::vector<TradeData> HistoricalDataLoader::filterBySymbol(
    const std::vector<TradeData>& data,
    const std::string& symbol) {
    
    std::vector<TradeData> filtered;
    filtered.reserve(data.size());
    
    for (const auto& trade : data) {
        if (trade.symbol == symbol) {
            filtered.push_back(trade);
        }
    }
    
    return filtered;
}

std::vector<TradeData> HistoricalDataLoader::filterByTimeRange(
    const std::vector<TradeData>& data,
    int64_t startTime,
    int64_t endTime) {
    
    std::vector<TradeData> filtered;
    filtered.reserve(data.size());
    
    for (const auto& trade : data) {
        if (trade.timestamp >= startTime && trade.timestamp <= endTime) {
            filtered.push_back(trade);
        }
    }
    
    return filtered;
}

void HistoricalDataLoader::sortByTimestamp(std::vector<TradeData>& data) {
    std::sort(data.begin(), data.end(),
        [](const TradeData& a, const TradeData& b) {
            return a.timestamp < b.timestamp;
        });
}

int64_t HistoricalDataLoader::parseTimestamp(const std::string& timestampStr) {
    // Try to parse as Unix milliseconds
    try {
        return std::stoll(timestampStr);
    } catch (...) {
        // If that fails, could add ISO date parsing here
        throw std::runtime_error("Invalid timestamp format: " + timestampStr);
    }
}

std::string HistoricalDataLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

} // namespace engine
