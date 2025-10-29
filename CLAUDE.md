# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
This is a C++ event scheduling application (also known as a 'step sequencer' application) called micro-composer. The project follows modern C++ standards and best practices. It aims to take full advantage of modern C++ concurrency control features and of a machine's available CPU cores if multiple cores are available. It aims to be a 'universal sequencer': to be able to schedule as wide a range of event types as possible, so that it may be used in a wide range of disciplines. Examples of intended use cases are audio programming, high-frequency trading, physics experiments and simulations. Optimal timing accuracy is a high priority in this project.

## Initial focus
To begin with, the main focus is on MIDI sequencing since this is provides a straightforward, fun way for humans to get immediate audible feedback on the timing accuracy of the sequencer. The sequencer is designed to be used as a library that can be integrated into larger applications, as well as a standalone application with a GUI.

## Lock-free use case

My specific actual use case is to have a Sequencer producing events with near perfect timing-accuracy, while at the same time have those events, except for the one event currently
being scheduled i.e. the event 'under playhead', to be modifiable on the fly (while the Sequencer is actively scheduling them in the `scheduler_` thread). So that's why I'm searching for
 ways to give the `scheduler_` thread lock-free (or as close to lock-free as is safe) priority access to the Sequencer's `events_` container. If it can't be done lock-free, then at least
 I want the on-the-fly updates to never interfere with the performance of the `scheduler_` thread, so I never want the scheduler thread to be waiting on locks held by on-the-fly update
functions, while on the other hand I'm perfectly fine with the on-the-fly update functions having a small performance penalty because they are waiting on locks held by the scheduler
thread or on lock-free thread-safety mechanisms used by the scheduler thread. I want my scheduler thread to be able to do 2 things without any competition from other threads: 1. read the
 next event from `events_` so that it can get the duration and 2. push that event to the output queue `output_`.

## Build System
- Use CMake as the primary build system
- Standard commands:
  - `mkdir build && cd build && cmake .. && make` - Initial build
  - `make` - Incremental build from build directory
  - `make clean` - Clean build artifacts
  - `ctest` - Run tests (when test suite is implemented)

## C++ Standards and Guidelines
- Use C++20 or later standard features
- Follow RAII (Resource Acquisition Is Initialization) principles
- Use smart pointers (std::unique_ptr, std::shared_ptr) instead of raw pointers
- Prefer const-correctness throughout the codebase
- Use namespace organization to prevent naming conflicts
- Follow the Rule of Five for classes managing resources
- Use STL containers and algorithms where appropriate
- ALWAYS make sure you do test driven development and follow the SOLID principles

## Unit Tests
- Use Catch2 as testing framework
- tests are in the 'tests/' directory
- tests directory has its own CMakeLists.txt
- Write tests for all public interfaces and critical internal logic

## Audio Programming Specifics
- Real-time audio code must be lock-free and allocation-free
- Separate audio thread logic from UI/control thread logic
- Use double-buffering or lock-free queues for thread communication
- Audio buffer sizes should be powers of 2 when possible
- Sample rate and buffer size should be configurable
- Use fixed-point arithmetic for tempo/timing calculations where precision matters

## Code Organization
- Header files (.h) for declarations
- Source files (.cpp) for implementations
- Template implementations in .tpp files
- Separate directories for different subsystems (sequencable, sequencer, container, concurrency, utility, etc.)
- Use forward declarations in headers to minimize compilation dependencies
- Keep headers minimal and include only what's necessary

## Memory Management
- Avoid dynamic allocation in real-time audio callbacks
- Pre-allocate buffers and data structures during initialization
- Use stack allocation or pre-allocated pools for temporary objects
- Be mindful of cache locality for performance-critical code

## Error Handling
- Use exceptions for non-recoverable errors during initialization
- Use error codes or optional types for recoverable errors in audio code
- Never throw exceptions from real-time audio callbacks
- Validate input parameters and provide meaningful error messages

## Threading Considerations
- Audio processing should run on a dedicated high-priority thread
- Use atomic types for simple shared state between threads
- Implement proper synchronization for complex shared data
- Avoid blocking operations in audio threads

## Development Environment
- Using neovim as the primary code editor
- Using clangd as the language server for C++ code completion and linting
- running on Arch Linux with a real-time kernel
- using pipewire

## Dependencies
- Minimize external dependencies
- When using audio libraries, prefer cross-platform options (libremidi, JUCE, RtAudio, PortAudio)
- Use header-only libraries when possible to simplify builds
- Vendor critical dependencies or use git submodules for reproducible builds

## Git Workflow
- Follow git conventional commits syntax and style (see https://conventionalcommits.org for reference)
- ANYTIME you to make a todo-list to accomplish a task I give you, make SEPARATE git commits for EACH item in the todo-list (but only if the item includes code changes)
- Do NOT commit too many changes at once
- Each time you finish writing the code for a new class, type or interface, write unit tests for it, recompile and test until it works, and then make a git commit with the message 'feat: new class Name', replacing 'Name' with the actual name of the class. This commit should include the class declaration, definition and the unit tests for it.
- make a separate git commit for each set of changes that is as small as possible, such that each committed set of changes starts from a fully functioning codebase and results again in a fully functional codebase

## GUI (Graphical User Interface)
- code @include/gui/gui.h and @include/gui/gui.tpp
- using GTK
- GUI uses a `Poly_sequencer_controller` (@include/controller/poly_sequencer_controller.h) to interact with several sequencers running in parallel
- Each sequencer is represented in the GUI by a `Sequencer_widget`. This GTK widget displays a grid where column indices map to event indices in the Sequencer's events container, whereas row indices map to different parameters of each event (e.g. enabled, duration, frequency, etc.).
- Edits to cell values displayed in the GUI are applied to the underlying Sequencer's events container via the `Poly_sequencer::mutate(Seq_idx, Event_idx, Mutator)` function, which is inherited by the `Poly_sequencer_controller` from the `Poly_sequencer` class.
### Mapping Sequencer Widget Rows to Event Parameters
- We do not know in advance which parameters (editable data members) the event type passed to the Gui as a template parameter will have. This gives rise to the problem: How to know which row corresponds to which parameter of the event type? Proposed solutions:
- Use a traits class called `Event_traits<Event>` that the user of the Gui must specialize for their specific event type. This traits class provides static functions that map row indices to event parameters, and vice versa. It also provides functions to get and set parameter values given an event instance and a row index.
- Use a `Tuple_event` class that wraps a `std::tuple` of parameter values. This class provides methods to get and set parameter values by index, making it easier to map row indices to tuple indices. This `Tuple_event` class then would also need to provide a way to get labels and units for each parameter, for instance via static methods or a separate traits class.
- Use reflection (if available) to dynamically inspect the event type and determine its parameters. This would require more complex code and may have performance implications, so it is less preferred than other approaches.

