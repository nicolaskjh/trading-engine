#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace engine {

/**
 * Config: Simple configuration system for trading engine
 * 
 * Supports loading from JSON-style config files with hierarchical keys.
 * Uses dot notation for nested values: "portfolio.max_position_size"
 */
class Config {
public:
    /**
     * Load configuration from file
     */
    static bool loadFromFile(const std::string& filename);

    /**
     * Get string value with optional default
     */
    static std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * Get integer value with optional default
     */
    static int getInt(const std::string& key, int defaultValue = 0);

    /**
     * Get double value with optional default
     */
    static double getDouble(const std::string& key, double defaultValue = 0.0);

    /**
     * Get boolean value with optional default
     */
    static bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * Set a value programmatically
     */
    static void set(const std::string& key, const std::string& value);

    /**
     * Check if a key exists
     */
    static bool has(const std::string& key);

    /**
     * Clear all configuration
     */
    static void clear();

    /**
     * Get all keys (for debugging/testing)
     */
    static std::unordered_map<std::string, std::string> getAll();

private:
    static std::unordered_map<std::string, std::string> values_;
};

} // namespace engine
