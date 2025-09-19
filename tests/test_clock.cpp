#include "clock.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>

using namespace MicroComposer::sequence;

TEST_CASE("Sequence_clock basic functionality", "[clock]") {
  Sequence_clock clock;

  SECTION("Clock starts in stopped state") { REQUIRE_FALSE(clock.is_live()); }

  SECTION("Clock default interval is 1000ms") {
    REQUIRE(clock.get_interval() == std::chrono::milliseconds(1000));
  }

  SECTION("Clock interval can be changed") {
    auto new_interval = max(clock.timing_capabilities.min_interval,
                            std::chrono::milliseconds(500));
    clock.set_interval(new_interval);
    REQUIRE(clock.get_interval() == new_interval);
  }
}

TEST_CASE("Sequence_clock start/stop operations", "[clock]") {
  Sequence_clock clock;

  SECTION("Clock can be started and stopped") {
    clock.start();
    REQUIRE(clock.is_live());
    clock.stop();
    REQUIRE_FALSE(clock.is_live());
  }

  SECTION("Starting already started clock is safe") {
    clock.start();
    REQUIRE(clock.is_live());
    clock.start(); // Should not cause issues
    REQUIRE(clock.is_live());
    clock.stop();
  }

  SECTION("Stopping already stopped clock is safe") {
    REQUIRE_FALSE(clock.is_live());
    clock.stop(); // Should not cause issues
    REQUIRE_FALSE(clock.is_live());
  }
}

TEST_CASE("Sequence_clock timing behavior", "[clock][timing]") {
  Sequence_clock clock;

  SECTION("Clock is precise enough for minimum allowed interval") {
    auto test_interval = clock.timing_capabilities.min_interval;
    clock.set_interval(test_interval);

    clock.start();

    // Wait for first tick to synchronize
    clock.wait();

    auto start_time = std::chrono::steady_clock::now();

    const uint n_ticks = 60;

    // Wait for 2 more ticks
    for (uint i = 0; i < n_ticks; ++i) {
      clock.wait();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = end_time - start_time;

    clock.stop();

    // Should be approximately n_ticks intervals (with tolerance for timing
    // precision)
    auto tolerance = clock.timing_capabilities.precision;
    decltype(tolerance) expected = n_ticks * test_interval;

    REQUIRE(elapsed >= (expected - tolerance));
    REQUIRE(elapsed <= (expected + tolerance));
  }
}