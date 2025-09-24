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

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0)

# Add original console executable
add_executable(MicroComposer
    src/main.cpp
    src/synth/synth_output.cpp
)

# Add GUI executable (only if GTKmm is found)
if(GTKMM_FOUND)
    add_executable(MicroComposerGUI
        src/main_gui.cpp
        src/synth/synth_output.cpp
        src/graphical/sequencer_controller.cpp
        src/graphical/step_box_widget.cpp
        src/graphical/step_grid_widget.cpp
        src/graphical/step_parameter_menu.cpp
        src/graphical/main_window.cpp
    )

    # Link GTKmm libraries
    target_link_libraries(MicroComposerGUI ${GTKMM_LIBRARIES})
    target_include_directories(MicroComposerGUI PRIVATE ${GTKMM_INCLUDE_DIRS})
    target_compile_options(MicroComposerGUI PRIVATE ${GTKMM_CFLAGS_OTHER})
else()
    message(WARNING "GTKmm not found. GUI will not be built. Install libgtkmm-3.0-dev to enable GUI.")
endif()

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

