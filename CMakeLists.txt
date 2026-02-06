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
    src/data/BookManager.cpp
    src/data/MarketDataHandler.cpp
    src/data/OrderBook.cpp
    src/order/OrderLogger.cpp
    src/logger/Logger.cpp
    src/risk/Portfolio.cpp
)

# Main executable - Trading Engine
add_executable(trading_engine
    main.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Event System Tests
add_executable(test_event
    tests/event_test.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Order Manager Tests
add_executable(test_ordermanager
    tests/ordermanager_test.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Order Book Tests
add_executable(test_orderbook
    tests/orderbook_test.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Book Manager Tests
add_executable(test_bookmanager
    tests/bookmanager_test.cpp
    ${ENGINE_SOURCES}
)

# Test executable - Portfolio Tests
add_executable(test_portfolio
    tests/portfolio_test.cpp
    ${ENGINE_SOURCES}
)

# Link threading library (required for std::thread, std::mutex)
find_package(Threads REQUIRED)
target_link_libraries(trading_engine Threads::Threads)
target_link_libraries(test_event Threads::Threads)
target_link_libraries(test_ordermanager Threads::Threads)
target_link_libraries(test_orderbook Threads::Threads)
target_link_libraries(test_bookmanager Threads::Threads)
target_link_libraries(test_portfolio Threads::Threads)

# Enable testing
enable_testing()

# Register tests with CTest
add_test(NAME EventTests COMMAND test_event)
add_test(NAME OrderManagerTests COMMAND test_ordermanager)
add_test(NAME OrderBookTests COMMAND test_orderbook)
add_test(NAME BookManagerTests COMMAND test_bookmanager)
add_test(NAME PortfolioTests COMMAND test_portfolio)

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "C++ flags (Debug): ${CMAKE_CXX_FLAGS_DEBUG}")
