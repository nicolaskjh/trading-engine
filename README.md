# Trading Engine

High-frequency algorithmic trading engine for US equities, built in C++17. The engine uses an event-driven architecture to process market data, manage orders, and execute trading strategies with low latency.

## Project Structure

```
trading-engine/
├── include/           # Header files (event system, orders, positions)
├── src/               # Implementation files
├── tests/             # Test files and component validation
├── main.cpp           # Engine entry point
├── CMakeLists.txt     # Build configuration
└── README.md
```

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .
```

## Running

```bash
# Run the trading engine
./trading_engine

# Run component tests
./test_all
```

## Build Configuration

- **Release** (default): Optimized for performance (-O3 -march=native)
- **Debug**: Debug symbols and warnings (-g -Wall -Wextra)

To build in Debug mode:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- POSIX threads
