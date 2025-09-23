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

# Add original console executable
add_executable(MicroComposer
    src/main.cpp
    src/synth/synth_output.cpp
)

# Add GUI executable
add_executable(MicroComposerGUI
    src/main_gui.cpp
    src/synth/synth_output.cpp
    src/graphical/gui_application.cpp
    src/graphical/main_window.cpp
    src/client/gui_client.cpp
)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

