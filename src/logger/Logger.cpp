#include "logger/Logger.h"

namespace engine {

// Static member initialization
LogLevel Logger::minLevel_ = LogLevel::INFO;
std::mutex Logger::mutex_;
std::ofstream Logger::logFile_;
bool Logger::initialized_ = false;

void Logger::init(LogLevel minLevel, const std::string& logFile) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    minLevel_ = minLevel;
    initialized_ = true;
    
    if (!logFile.empty()) {
        logFile_.open(logFile, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << logFile << "\n";
        }
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

// Enum component overloads
void Logger::debug(LogComponent component, const std::string& message) {
    log(LogLevel::DEBUG, getComponentString(component), message);
}

void Logger::info(LogComponent component, const std::string& message) {
    log(LogLevel::INFO, getComponentString(component), message);
}

void Logger::warning(LogComponent component, const std::string& message) {
    log(LogLevel::WARNING, getComponentString(component), message);
}

void Logger::error(LogComponent component, const std::string& message) {
    log(LogLevel::ERROR, getComponentString(component), message);
}

void Logger::critical(LogComponent component, const std::string& message) {
    log(LogLevel::CRITICAL, getComponentString(component), message);
}

// String component overloads
void Logger::debug(const std::string& component, const std::string& message) {
    log(LogLevel::DEBUG, component, message);
}

void Logger::info(const std::string& component, const std::string& message) {
    log(LogLevel::INFO, component, message);
}

void Logger::warning(const std::string& component, const std::string& message) {
    log(LogLevel::WARNING, component, message);
}

void Logger::error(const std::string& component, const std::string& message) {
    log(LogLevel::ERROR, component, message);
}

void Logger::critical(const std::string& component, const std::string& message) {
    log(LogLevel::CRITICAL, component, message);
}

void Logger::log(LogLevel level, const std::string& component, 
                const std::string& message) {
    // Check if this log level should be output
    if (level < minLevel_) {
        return;
    }
    
    writeLog(level, component, message);
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default:                 return "UNKNOWN";
    }
}
std::string Logger::getComponentString(LogComponent component) {
    switch (component) {
        case LogComponent::ENGINE: return "Engine";
        case LogComponent::TEST: return "Test";
        case LogComponent::MARKET_DATA: return "MarketData";
        case LogComponent::MARKET_DATA_HANDLER: return "MarketDataHandler";
        case LogComponent::ORDER_LOGGER: return "OrderLogger";
        case LogComponent::ORDER_MANAGER: return "OrderManager";
        case LogComponent::SYSTEM: return "System";
        case LogComponent::TIMER: return "Timer";
        default: return "Unknown";
    }
}
std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    
    return ss.str();
}

void Logger::writeLog(LogLevel level, const std::string& component, 
                     const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "[" << getTimestamp() << "] "
       << "[" << getLevelString(level) << "] "
       << "[" << component << "] "
       << message;
    
    std::string logLine = ss.str();
    
    // Write to console
    if (level >= LogLevel::ERROR) {
        std::cerr << logLine << std::endl;
    } else {
        std::cout << logLine << std::endl;
    }
    
    // Write to file if open
    if (logFile_.is_open()) {
        logFile_ << logLine << std::endl;
        logFile_.flush();
    }
}

} // namespace engine
