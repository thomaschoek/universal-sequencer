#include "timing_config.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>

using namespace MicroComposer::timing_capabilities;

TEST_CASE("TimingCapabilities singleton behavior", "[timing_capabilities]") {
  SECTION("get_instance returns same instance") {
    const auto &instance1 = TimingCapabilities::get_instance();
    const auto &instance2 = TimingCapabilities::get_instance();

    REQUIRE(&instance1 == &instance2);
  }

  SECTION("singleton values are consistent") {
    const auto &instance1 = TimingCapabilities::get_instance();
    const auto &instance2 = TimingCapabilities::get_instance();

    REQUIRE(instance1.precision == instance2.precision);
    REQUIRE(instance1.min_interval == instance2.min_interval);
    REQUIRE(instance1.scheduling_quantum == instance2.scheduling_quantum);
  }
}

TEST_CASE("TimingCapabilities basic properties", "[timing_capabilities]") {
  const auto &capabilities = TimingCapabilities::get_instance();

  SECTION("precision is positive") {
    REQUIRE(capabilities.precision.count() > 0);
  }

  SECTION("min_interval is positive") {
    REQUIRE(capabilities.min_interval.count() > 0);
  }

  SECTION("scheduling_quantum is positive") {
    REQUIRE(capabilities.scheduling_quantum.count() > 0);
  }

  SECTION("min_interval is reasonable for system timing") {
    // Min interval should be at least 1ms but less than 1 second
    REQUIRE(capabilities.min_interval >= std::chrono::milliseconds(1));
    REQUIRE(capabilities.min_interval <= std::chrono::milliseconds(1000));
  }

  SECTION("precision is reasonable for nanosecond timing") {
    // Precision should be less than 1 second
    REQUIRE(capabilities.precision < std::chrono::seconds(1));
  }

  SECTION("scheduling_quantum is reasonable") {
    // Scheduling quantum should be between 1ms and 100ms for most systems
    REQUIRE(capabilities.scheduling_quantum >= std::chrono::milliseconds(1));
    REQUIRE(capabilities.scheduling_quantum <= std::chrono::milliseconds(100));
  }
}

TEST_CASE("TimingCapabilities relationship between values",
          "[timing_capabilities]") {
  const auto &capabilities = TimingCapabilities::get_instance();

  SECTION("min_interval is larger than precision") {
    // Convert to same units for comparison
    auto precision_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        capabilities.precision);
    REQUIRE(capabilities.min_interval > precision_ms);
  }

  SECTION("min_interval considers scheduling_quantum") {
    // Min interval should be at least as large as the scheduling quantum
    auto quantum_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        capabilities.scheduling_quantum);
    REQUIRE(capabilities.min_interval >= quantum_ms);
  }
}

TEST_CASE("TimingCapabilities constants are reasonable",
          "[timing_capabilities]") {
  SECTION("test_iterations is reasonable") {
    REQUIRE(test_iterations > 0);
    REQUIRE(test_iterations <= 10000); // Should complete in reasonable time
  }

  SECTION("test_sleep_time is reasonable") {
    REQUIRE(test_sleep_time.count() > 0);
    REQUIRE(test_sleep_time <=
            std::chrono::milliseconds(10)); // Should be short for testing
  }
}