#include "config/Config.h"

#include <cassert>
#include <fstream>
#include <iostream>

using namespace engine;

// Test file content helper
void createTestConfigFile(const std::string& filename) {
    std::ofstream file(filename);
    file << "# Test configuration\n";
    file << "[database]\n";
    file << "host = localhost\n";
    file << "port = 5432\n";
    file << "timeout = 30.5\n";
    file << "enabled = true\n";
    file << "\n";
    file << "[strategy]\n";
    file << "name = \"TestStrategy\"  # quoted value\n";
    file << "symbols: AAPL, TSLA  // colon separator\n";
    file << "threshold = 0.05\n";
    file << "active = yes\n";
    file.close();
}

// Test 1: Load configuration file
void testLoadConfig() {
    std::cout << "Running testLoadConfig..." << std::endl;

    Config::clear();
    
    const std::string testFile = "test_config.ini";
    createTestConfigFile(testFile);

    bool loaded = Config::loadFromFile(testFile);
    assert(loaded);

    // Cleanup
    std::remove(testFile.c_str());

    std::cout << "PASSED: testLoadConfig" << std::endl;
}

// Test 2: Get string values
void testGetString() {
    std::cout << "Running testGetString..." << std::endl;

    Config::clear();
    
    const std::string testFile = "test_config.ini";
    createTestConfigFile(testFile);
    Config::loadFromFile(testFile);

    assert(Config::getString("database.host") == "localhost");
    assert(Config::getString("strategy.name") == "TestStrategy");
    assert(Config::getString("nonexistent", "default") == "default");

    std::remove(testFile.c_str());

    std::cout << "PASSED: testGetString" << std::endl;
}

// Test 3: Get integer values
void testGetInt() {
    std::cout << "Running testGetInt..." << std::endl;

    Config::clear();
    
    const std::string testFile = "test_config.ini";
    createTestConfigFile(testFile);
    Config::loadFromFile(testFile);

    assert(Config::getInt("database.port") == 5432);
    assert(Config::getInt("nonexistent", 42) == 42);

    std::remove(testFile.c_str());

    std::cout << "PASSED: testGetInt" << std::endl;
}

// Test 4: Get double values
void testGetDouble() {
    std::cout << "Running testGetDouble..." << std::endl;

    Config::clear();
    
    const std::string testFile = "test_config.ini";
    createTestConfigFile(testFile);
    Config::loadFromFile(testFile);

    double timeout = Config::getDouble("database.timeout");
    assert(timeout > 30.4 && timeout < 30.6);
    
    double threshold = Config::getDouble("strategy.threshold");
    assert(threshold > 0.04 && threshold < 0.06);
    
    assert(Config::getDouble("nonexistent", 3.14) == 3.14);

    std::remove(testFile.c_str());

    std::cout << "PASSED: testGetDouble" << std::endl;
}

// Test 5: Get boolean values
void testGetBool() {
    std::cout << "Running testGetBool..." << std::endl;

    Config::clear();
    
    const std::string testFile = "test_config.ini";
    createTestConfigFile(testFile);
    Config::loadFromFile(testFile);

    assert(Config::getBool("database.enabled") == true);
    assert(Config::getBool("strategy.active") == true);
    assert(Config::getBool("nonexistent", false) == false);

    std::remove(testFile.c_str());

    std::cout << "PASSED: testGetBool" << std::endl;
}

// Test 6: Set values programmatically
void testSetValue() {
    std::cout << "Running testSetValue..." << std::endl;

    Config::clear();
    
    Config::set("test.key", "test_value");
    assert(Config::getString("test.key") == "test_value");
    
    Config::set("test.number", "123");
    assert(Config::getInt("test.number") == 123);

    std::cout << "PASSED: testSetValue" << std::endl;
}

// Test 7: Check key existence
void testHasKey() {
    std::cout << "Running testHasKey..." << std::endl;

    Config::clear();
    
    Config::set("existing.key", "value");
    assert(Config::has("existing.key") == true);
    assert(Config::has("nonexistent.key") == false);

    std::cout << "PASSED: testHasKey" << std::endl;
}

// Test 8: Load main config file
void testLoadMainConfig() {
    std::cout << "Running testLoadMainConfig..." << std::endl;

    Config::clear();
    
    // Load the actual config file
    bool loaded = Config::loadFromFile("../config.ini");
    
    if (loaded) {
        // Verify some expected values
        assert(Config::getDouble("portfolio.initial_capital") == 1000000.0);
        assert(Config::getInt("exchange.fill_latency_ms") == 10);
        assert(Config::getInt("strategy.sma.fast_period") == 10);
        assert(Config::getString("logging.level") == "info");
        
        std::cout << "PASSED: testLoadMainConfig" << std::endl;
    } else {
        std::cout << "SKIPPED: testLoadMainConfig (config.ini not found)" << std::endl;
    }
}

// Test 9: Clear configuration
void testClear() {
    std::cout << "Running testClear..." << std::endl;

    Config::clear();
    Config::set("test.key", "value");
    assert(Config::has("test.key"));
    
    Config::clear();
    assert(!Config::has("test.key"));

    std::cout << "PASSED: testClear" << std::endl;
}

// Test 10: Get all values
void testGetAll() {
    std::cout << "Running testGetAll..." << std::endl;

    Config::clear();
    Config::set("key1", "value1");
    Config::set("key2", "value2");
    
    auto all = Config::getAll();
    assert(all.size() == 2);
    assert(all["key1"] == "value1");
    assert(all["key2"] == "value2");

    std::cout << "PASSED: testGetAll" << std::endl;
}

int main() {
    std::cout << "=== Running Config Tests ===" << std::endl;

    testLoadConfig();
    testGetString();
    testGetInt();
    testGetDouble();
    testGetBool();
    testSetValue();
    testHasKey();
    testLoadMainConfig();
    testClear();
    testGetAll();

    std::cout << "\n=== All Config Tests Passed ===" << std::endl;
    return 0;
}
