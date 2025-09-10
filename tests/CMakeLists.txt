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
    sequence/test_clock.cpp
)

# Link against Catch2
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Include project headers
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Add tests to CTest
include(CTest)
include(Catch)
catch_discover_tests(tests)