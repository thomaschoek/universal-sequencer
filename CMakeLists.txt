# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MicroComposer VERSION 1.0.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
include_directories(include)

# Add executable
add_executable(MicroComposer src/main.cpp)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

