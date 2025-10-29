# Minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Project name and version
project(MicroComposer VERSION 1.0.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch libremidi
include(FetchContent)
FetchContent_Declare(
  libremidi
  GIT_REPOSITORY https://github.com/celtera/libremidi.git
  GIT_TAG v5.3.1
)
FetchContent_MakeAvailable(libremidi)

# Find GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Include directories
include_directories(include ${GTK3_INCLUDE_DIRS})

# Add original console executable
add_executable(MicroComposerCLI
    src/main.cpp
    src/synth/synth_output.cpp
)

# Link GTK3 and glib
target_link_libraries(MicroComposerCLI ${GTK3_LIBRARIES})
target_link_directories(MicroComposerCLI PRIVATE ${GTK3_LIBRARY_DIRS})
target_compile_options(MicroComposerCLI PRIVATE ${GTK3_CFLAGS_OTHER})

# Add GUI executable
add_executable(MicroComposer
    src/main_gui.cpp
    src/synth/synth_output.cpp
)

# Link GTK3 and glib
target_link_libraries(MicroComposer ${GTK3_LIBRARIES})
target_link_directories(MicroComposer PRIVATE ${GTK3_LIBRARY_DIRS})
target_compile_options(MicroComposer PRIVATE ${GTK3_CFLAGS_OTHER})

# Add MIDI executable
add_executable(MicroComposerMIDI
    src/main_midi.cpp
    src/midi/midi_output.cpp
    src/sequencable/midi_event.cpp
)

# Link libremidi and required system libraries
target_link_libraries(MicroComposerMIDI libremidi asound pthread)

# Add MIDI GUI executable
add_executable(midiguy
    src/main_midi_gui.cpp
    src/midi/midi_output.cpp
    src/sequencable/midi_event.cpp
)

# Link libremidi and required system libraries
target_link_libraries(midiguy libremidi asound pthread)

# Link GTK3 and glib
target_link_libraries(midiguy ${GTK3_LIBRARIES})
target_link_directories(midiguy PRIVATE ${GTK3_LIBRARY_DIRS})
target_compile_options(midiguy PRIVATE ${GTK3_CFLAGS_OTHER})

# Copy CSS resources to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/gui_style.css
    ${CMAKE_CURRENT_BINARY_DIR}/resources/gui_style.css
    COPYONLY
)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

