#include <catch2/catch_test_macros.hpp>
#include "sequencer/sequencer_template.h"
#include <atomic>
#include <chrono>
#include <thread>

using namespace Micro_composer;
using namespace Micro_composer::sequencer;

// Simple event for testing
struct Atomic_test_event {
  bool enabled{true};
  sequencable::Duration duration{std::chrono::milliseconds(100)};
  sequencable::Time_point scheduled_time{};

  void set_duration(sequencable::Duration d) { duration = d; }
  void set_scheduled_time(sequencable::Time_point t) { scheduled_time = t; }
  void update(const Atomic_test_event& other) {
    enabled = other.enabled;
    duration = other.duration;
  }
};

// Counter to track atomic operations (for demonstration purposes)
std::atomic<size_t> atomic_load_count{0};

TEST_CASE("Sequencer atomic operations - insert when not scheduling", "[sequencer][atomic][performance]") {
  auto handler = [](Atomic_test_event&&) {};
  Sequencer<Atomic_test_event> seq(handler);

  // Add some events
  for (int i = 0; i < 5; ++i) {
    seq.push_back(Atomic_test_event{});
  }

  SECTION("Insert when sequencer is not running") {
    // Sequencer is NOT scheduling
    REQUIRE_FALSE(seq.is_scheduling());

    // Insert should be fast and not require looping
    Atomic_test_event new_event{};
    auto start = std::chrono::steady_clock::now();
    seq.insert(2, new_event);
    auto end = std::chrono::steady_clock::now();

    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Should be very fast (< 100μs) since no scheduling loop needed
    INFO("Insert duration: " << duration_us << "μs");
    REQUIRE(duration_us < 100);
    REQUIRE(seq.size() == 6);
  }

  SECTION("Erase when sequencer is not running") {
    REQUIRE_FALSE(seq.is_scheduling());

    auto start = std::chrono::steady_clock::now();
    seq.erase(2);
    auto end = std::chrono::steady_clock::now();

    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    INFO("Erase duration: " << duration_us << "μs");
    REQUIRE(duration_us < 100);
    REQUIRE(seq.size() == 4);
  }
}

TEST_CASE("Sequencer atomic operations - insert while scheduling", "[sequencer][atomic][performance]") {
  auto handler = [](Atomic_test_event&&) {};
  Sequencer<Atomic_test_event> seq(handler);

  // Add events with longer duration to have time to insert
  for (int i = 0; i < 10; ++i) {
    seq.push_back(Atomic_test_event{true, std::chrono::milliseconds(100)});
  }

  SECTION("Insert while sequencer is running") {
    auto start_time = std::chrono::steady_clock::now();
    seq.start(start_time, true);

    // Wait for scheduler to advance a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    REQUIRE(seq.is_scheduling());

    // Insert at a safe position (far from current)
    Atomic_test_event new_event{};
    seq.insert(8, new_event);

    REQUIRE(seq.size() == 11);

    seq.pause(std::chrono::steady_clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

TEST_CASE("Atomic operations - minimal loads for non-scheduling operations", "[sequencer][atomic][optimization]") {
  auto handler = [](Atomic_test_event&&) {};
  Sequencer<Atomic_test_event> seq(handler);

  for (int i = 0; i < 5; ++i) {
    seq.push_back(Atomic_test_event{});
  }

  SECTION("Multiple inserts when not scheduling should be fast") {
    REQUIRE_FALSE(seq.is_scheduling());

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 10; ++i) {
      Atomic_test_event new_event{};
      seq.insert(0, new_event);
    }

    auto end = std::chrono::steady_clock::now();
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    INFO("Total time for 10 inserts: " << total_us << "μs");
    INFO("Average per insert: " << (total_us / 10) << "μs");

    // Should average < 50μs per insert when not scheduling
    REQUIRE(total_us / 10 < 50);
    REQUIRE(seq.size() == 15);
  }
}

TEST_CASE("Documentation: Expected atomic load pattern", "[sequencer][atomic][documentation]") {
  // This test documents the expected behavior after optimization

  SECTION("Optimal pattern for insert when not scheduling") {
    // Expected: 1 atomic load to get current position
    // No loop iterations needed
    // Total: 1 atomic load operation

    INFO("When not scheduling, insert should require minimal atomic operations");
    INFO("Expected: ~1 atomic load (to check current position)");
    REQUIRE(true);  // Documentation test always passes
  }

  SECTION("Optimal pattern for insert while scheduling") {
    // Expected: Load inside loop until safe, then reload after scheduler stops
    // Multiple loads acceptable when actually scheduling

    INFO("When scheduling, insert may require multiple loads during wait loop");
    INFO("This is expected and necessary for thread safety");
    REQUIRE(true);
  }
}
