#include "config/Config.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace engine {

std::unordered_map<std::string, std::string> Config::values_;

// Helper functions
namespace {
    std::string trim(const std::string& str) {
        auto start = str.begin();
        while (start != str.end() && std::isspace(*start)) {
            start++;
        }

        auto end = str.end();
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));

        return std::string(start, end + 1);
    }

    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && 
               str.compare(0, prefix.size(), prefix) == 0;
    }

    std::string stripQuotes(const std::string& str) {
        std::string result = trim(str);
        if (result.size() >= 2) {
            if ((result.front() == '"' && result.back() == '"') ||
                (result.front() == '\'' && result.back() == '\'')) {
                return result.substr(1, result.size() - 2);
            }
        }
        return result;
    }
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || startsWith(line, "//")) {
            continue;
        }

        // Handle sections [section_name]
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            currentSection = trim(currentSection);
            continue;
        }

        // Handle key=value or key: value pairs
        size_t delimPos = line.find('=');
        if (delimPos == std::string::npos) {
            delimPos = line.find(':');
        }

        if (delimPos != std::string::npos) {
            std::string key = trim(line.substr(0, delimPos));
            std::string value = trim(line.substr(delimPos + 1));
            
            // Remove trailing comments
            size_t commentPos = value.find('#');
            if (commentPos == std::string::npos) {
                commentPos = value.find("//");
            }
            if (commentPos != std::string::npos) {
                value = trim(value.substr(0, commentPos));
            }

            // Strip quotes from value
            value = stripQuotes(value);

            // Build full key with section prefix
            std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            values_[fullKey] = value;
        }
    }

    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
    }
    return defaultValue;
}

void Config::set(const std::string& key, const std::string& value) {
    values_[key] = value;
}

bool Config::has(const std::string& key) {
    return values_.find(key) != values_.end();
}

void Config::clear() {
    values_.clear();
}

std::unordered_map<std::string, std::string> Config::getAll() {
    return values_;
}

} // namespace engine
