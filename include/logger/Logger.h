#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace engine {

/**
 * LogLevel: Severity levels for log messages
 */
enum class LogLevel {
    DEBUG,      // Detailed information for debugging
    INFO,       // General informational messages
    WARNING,    // Warning messages
    ERROR,      // Error messages
    CRITICAL    // Critical errors
};

/**
 * LogComponent: Component identifiers for logging
 */
enum class LogComponent {
    ENGINE,
    TEST,
    MARKET_DATA,
    MARKET_DATA_HANDLER,
    ORDER_LOGGER,
    ORDER_MANAGER,
    SYSTEM,
    TIMER
};

/**
 * Logger: Thread-safe logging system with configurable output and log levels.
 * Provides timestamped, level-based logging to console and/or file.
 */
class Logger {
public:
    /**
     * Initialize the logger with optional file output
     */
    static void init(LogLevel minLevel = LogLevel::INFO, 
                    const std::string& logFile = "");
    
    /**
     * Set minimum log level
     */
    static void setLogLevel(LogLevel level);
    
    /**
     * Log messages at different levels (enum component overloads)
     */
    static void debug(LogComponent component, const std::string& message);
    static void info(LogComponent component, const std::string& message);
    static void warning(LogComponent component, const std::string& message);
    static void error(LogComponent component, const std::string& message);
    static void critical(LogComponent component, const std::string& message);
    
    /**
     * Log messages at different levels (string component overloads)
     */
    static void debug(const std::string& component, const std::string& message);
    static void info(const std::string& component, const std::string& message);
    static void warning(const std::string& component, const std::string& message);
    static void error(const std::string& component, const std::string& message);
    static void critical(const std::string& component, const std::string& message);
    
    /**
     * Generic log method
     */
    static void log(LogLevel level, const std::string& component, 
                   const std::string& message);
    
    /**
     * Close log file if open
     */
    static void shutdown();

private:
    Logger() = default;
    
    static std::string getLevelString(LogLevel level);
    static std::string getComponentString(LogComponent component);
    static std::string getTimestamp();
    static void writeLog(LogLevel level, const std::string& component, 
                        const std::string& message);
    
    static LogLevel minLevel_;
    static std::mutex mutex_;
    static std::ofstream logFile_;
    static bool initialized_;
};

} // namespace engine
