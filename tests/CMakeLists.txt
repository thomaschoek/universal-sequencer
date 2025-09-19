# Download and configure Catch2
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Create test executable
add_executable(tests
    main.cpp
    test_clock.cpp
    test_timing_capabilities.cpp
    test_atomic_step_sequence.cpp
    test_atomic_deque.cpp
    test_sequencer.cpp
)

# Link against Catch2 and project sources
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Add project source files that tests depend on
target_sources(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/sequence/atomic_step_sequence.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer_output.cpp
    ${CMAKE_SOURCE_DIR}/src/synth/synth.cpp
)

# Include project headers
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Add tests to CTest
include(CTest)
include(Catch)
catch_discover_tests(tests)