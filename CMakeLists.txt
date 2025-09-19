# Minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Project name and version
project(MicroComposer VERSION 1.0.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
include_directories(include)

# Add executable
add_executable(MicroComposer
    src/main.cpp
    src/sequencer/step_sequencer.cpp
    src/sequence/atomic_step_sequence.cpp
    src/synth/synth.cpp
    src/sequencer/step_sequencer_output.cpp
    src/sequencer/synth_sequencer.cpp
)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

# Add examples subdirectory
add_subdirectory(examples)

