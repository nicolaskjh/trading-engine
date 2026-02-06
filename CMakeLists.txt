cmake_minimum_required(VERSION 3.15)
project(TradingEngine VERSION 1.0 LANGUAGES CXX)

# Set C++17 standard (required for std::optional, structured bindings, etc.)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance
# These are critical for HFT systems
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic")

# Set default build type to Release for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(ENGINE_SOURCES
    src/market_data/BookManager.cpp
    src/market_data/MarketDataHandler.cpp
    src/market_data/OrderBook.cpp
    src/order/OrderLogger.cpp
    src/logger/Logger.cpp
)

# Main executable - Trading Engine
add_executable(trading_engine
    main.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Event System Tests
add_executable(test_event
    tests/test_event.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Order Manager Tests
add_executable(test_ordermanager
    tests/test_ordermanager.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Order Book Tests
add_executable(test_orderbook
    tests/test_orderbook.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Book Manager Tests
add_executable(test_bookmanager
    tests/test_bookmanager.cpp
    ${ENGINE_SOURCES}
)

# Test executable - All Tests Combined
add_executable(test_all
    tests/test_all.cpp
    tests/test_event.cpp
    tests/test_ordermanager.cpp
    tests/test_orderbook.cpp
    tests/test_bookmanager.cpp
    ${ENGINE_SOURCES}
)

# Define TEST_ALL for combined test executable to skip individual main() functions
target_compile_definitions(test_all PRIVATE TEST_ALL)

# Link threading library (required for std::thread, std::mutex)
find_package(Threads REQUIRED)
target_link_libraries(trading_engine Threads::Threads)
target_link_libraries(test_event Threads::Threads)
target_link_libraries(test_ordermanager Threads::Threads)
target_link_libraries(test_orderbook Threads::Threads)
target_link_libraries(test_bookmanager Threads::Threads)
target_link_libraries(test_all Threads::Threads)

# Enable testing
enable_testing()

# Register tests with CTest
add_test(NAME AllTests COMMAND test_all)
add_test(NAME EventTests COMMAND test_event)
add_test(NAME OrderManagerTests COMMAND test_ordermanager)
add_test(NAME OrderBookTests COMMAND test_orderbook)
add_test(NAME BookManagerTests COMMAND test_bookmanager)

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "C++ flags (Debug): ${CMAKE_CXX_FLAGS_DEBUG}")
