# micro-composer

A C++ MIDI step sequencer application that provides precise timing control for musical sequences.

## Design Philosophy

### Timing accuracy

Prioritize timing accuracy over data accuracy: it is more important that events are scheduled on time than that they reflect the latest edits made by users. So if a user is editing an event in a running sequence, we don't want the scheduler to have to wait for these edits to be complete before playing back the event. Rather, we just want it to load whatever value of the event is soonest available, whether it's the value before or after the edits, and play that back on time.

### On the fly edits
On-the-fly edit event values: we want to be able to edit the queued-up events even as the sequencer is running.
On-the-fly edits to event container: we want on-the-fly push, pop, resize, insert, erase etc. operations the data structure containing the events being sequenced.

Since we want on-the-fly edits to both the individual elements and their entire container data structures, barring some very clever atomic wizardry that for the time being we are not yet implementing, we need to lock the whole data structure in any operation that tries to load or write to an element. If we only needed on-the-fly edits to individual parameter values (so the container data structure itself remains constant, nothing is added/removed etc while running), then we would be safe with just `memory_order_relaxed` atomic loads and writes to those individual elements.

To combine these on-the-fly edits with the timing accuracy requirement, we use atomic stores and loads of the time at which the next event is to be scheduled (`Scheduler::t_next_`) in addition to conventional `std::scoped_lock`'s so that any edits made to a running sequencer will not cause interference during the critical time window in which the `Scheduler::runner_` thread needs immediate access to the data structure to maintain timing accuracy. See `Scheduler::await_runner_idle()` (@include/scheduler/scheduler.h, @src/scheduler/scheduler.cpp).

### Generic events

We want this sequencer to be generic; it should be able to schedule a wide variety of data types including user defined classes. This is why we need to work with containers of pointers to atomic pointers to the actual data types being scheduled (for instance, if we are using a vector, `std::vector<std::atomic<T*>*> sequence_`).

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