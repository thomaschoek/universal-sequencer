# micro-composer

A C++ MIDI step sequencer application that provides precise timing control for musical sequences.

## Build Instructions

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)

### Building the Project

1. **Clone and navigate to the project directory:**
   ```bash
   cd /path/to/universal-sequencer
   ```

2. **Create and enter build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Generate build files:**
   ```bash
   cmake ..
   ```

4. **Build the project:**
   ```bash
   make
   ```

### Build Commands Reference

- **Initial build:** `mkdir build && cd build && cmake .. && make`
- **Incremental build:** `make` (from build directory)
- **Clean build:** `make clean`
- **Run tests:** `ctest` (when test suite is implemented)

### Project Structure

- `include/` - Header files
- `src/` - Source files
- `build/` - Build artifacts (created during build)
- `CMakeLists.txt` - CMake configuration

### Testing

Directory Structure:
tests/
├── CMakeLists.txt           # Test build configuration
├── main.cpp                 # Test runner entry point
└── sequence/
    └── test_clock.cpp       # Unit tests for Sequence_clock

- Catch2 v3.4.0 integrated via CMake FetchContent (no manual installation needed)
- Comprehensive tests for Sequence_clock covering:
  - Basic state management (is_live, intervals)
  - Start/stop operations and edge cases
  - Timing precision validation
- CMake integration with CTest support
- Automatic test discovery - new tests are found automatically

Usage:
- cd build
- cmake .. && make - Build project and tests
- ./tests/tests - Run tests directly
- ctest - Run tests through CTest framework
- All tests currently pass (11 assertions across 3 test cases)

The framework is ready for expanding with additional test files following the same pattern.