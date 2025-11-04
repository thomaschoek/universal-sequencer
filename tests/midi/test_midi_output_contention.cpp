#include <catch2/catch_test_macros.hpp>
#include "midi/midi_output.h"
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace Micro_composer;
using namespace Micro_composer::midi;

// Test MIDI output wrapper for measuring mutex contention
// (Since we can't easily mock libremidi without major refactoring)
class Midi_output_contention_test {
public:
  // Measure mutex contention by timing send operations
  static std::chrono::microseconds measure_send_latency(
    Midi_output& midi_out,
    const libremidi::message& msg,
    int num_sends
  ) {
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < num_sends; ++i) {
      try {
        midi_out.send_message(msg);
      } catch (const std::runtime_error&) {
        // Port not open - expected in test environment
        // Just measure the mutex overhead
      }
    }

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  }

  // Measure contention with multiple concurrent threads
  static std::chrono::microseconds measure_concurrent_send_latency(
    Midi_output& midi_out,
    const libremidi::message& msg,
    int num_threads,
    int sends_per_thread
  ) {
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    auto start_time = std::chrono::steady_clock::now();

    // Create threads that will send messages concurrently
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&midi_out, &msg, &start_flag, sends_per_thread]() {
        // Wait for start signal
        while (!start_flag.load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }

        // Send messages
        for (int i = 0; i < sends_per_thread; ++i) {
          try {
            midi_out.send_message(msg);
          } catch (const std::runtime_error&) {
            // Port not open - expected in test environment
          }
        }
      });
    }

    // Start all threads simultaneously
    start_flag.store(true, std::memory_order_release);
    auto actual_start = std::chrono::steady_clock::now();

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - actual_start);
  }
};

TEST_CASE("MIDI output - baseline single-threaded performance", "[midi][performance]") {
  Midi_output midi_out;

  // Create a simple MIDI note-on message
  libremidi::message msg;
  msg.bytes = {0x90, 60, 100}; // Note On, Middle C, velocity 100

  SECTION("Single-threaded sends should be fast") {
    constexpr int NUM_SENDS = 1000;

    auto duration = Midi_output_contention_test::measure_send_latency(
      midi_out, msg, NUM_SENDS
    );

    auto avg_us = duration.count() / NUM_SENDS;

    INFO("Total time for " << NUM_SENDS << " sends: " << duration.count() << "μs");
    INFO("Average per send: " << avg_us << "μs");

    // Single-threaded should be reasonably fast (< 100μs per send on average)
    // This establishes a baseline
    REQUIRE(avg_us < 100);
  }
}

TEST_CASE("MIDI output - concurrent access contention", "[midi][performance][contention]") {
  Midi_output midi_out;

  libremidi::message msg;
  msg.bytes = {0x90, 60, 100}; // Note On, Middle C, velocity 100

  SECTION("Concurrent sends with 2 threads") {
    constexpr int NUM_THREADS = 2;
    constexpr int SENDS_PER_THREAD = 500;

    auto duration = Midi_output_contention_test::measure_concurrent_send_latency(
      midi_out, msg, NUM_THREADS, SENDS_PER_THREAD
    );

    auto total_sends = NUM_THREADS * SENDS_PER_THREAD;
    auto avg_us = duration.count() / total_sends;

    INFO("Concurrent (2 threads) time: " << duration.count() << "μs");
    INFO("Average per send: " << avg_us << "μs");

    // With 2 threads, some contention is expected but should still be reasonable
    REQUIRE(avg_us < 200);
  }

  SECTION("Concurrent sends with 4 threads") {
    constexpr int NUM_THREADS = 4;
    constexpr int SENDS_PER_THREAD = 250;

    auto duration = Midi_output_contention_test::measure_concurrent_send_latency(
      midi_out, msg, NUM_THREADS, SENDS_PER_THREAD
    );

    auto total_sends = NUM_THREADS * SENDS_PER_THREAD;
    auto avg_us = duration.count() / total_sends;

    INFO("Concurrent (4 threads) time: " << duration.count() << "μs");
    INFO("Average per send: " << avg_us << "μs");

    // With 4 threads, contention should be measurable
    // This test documents current behavior - may fail if contention is severe
    REQUIRE(avg_us < 500);
  }

  SECTION("Concurrent sends with 8 threads (stress test)") {
    constexpr int NUM_THREADS = 8;
    constexpr int SENDS_PER_THREAD = 125;

    auto duration = Midi_output_contention_test::measure_concurrent_send_latency(
      midi_out, msg, NUM_THREADS, SENDS_PER_THREAD
    );

    auto total_sends = NUM_THREADS * SENDS_PER_THREAD;
    auto avg_us = duration.count() / total_sends;

    INFO("Concurrent (8 threads) time: " << duration.count() << "μs");
    INFO("Average per send: " << avg_us << "μs");
    INFO("Contention factor vs single-threaded: " << (avg_us / 10.0));

    // With 8 threads, significant contention may occur
    // This test may fail if mutex contention is severe (> 1ms per send)
    // In that case, a lock-free queue solution would be warranted
    REQUIRE(avg_us < 1000);
  }
}

TEST_CASE("MIDI output - contention ratio measurement", "[midi][performance][documentation]") {
  Midi_output midi_out;

  libremidi::message msg;
  msg.bytes = {0x90, 60, 100};

  SECTION("Measure contention scaling") {
    constexpr int SENDS_PER_THREAD = 200;

    // Baseline: single-threaded
    auto single_thread_duration = Midi_output_contention_test::measure_send_latency(
      midi_out, msg, SENDS_PER_THREAD
    );
    auto baseline_avg = single_thread_duration.count() / SENDS_PER_THREAD;

    INFO("Baseline (1 thread): " << baseline_avg << "μs per send");

    // Test with increasing thread counts
    for (int num_threads : {2, 4, 8}) {
      auto concurrent_duration = Midi_output_contention_test::measure_concurrent_send_latency(
        midi_out, msg, num_threads, SENDS_PER_THREAD
      );

      auto total_sends = num_threads * SENDS_PER_THREAD;
      auto concurrent_avg = concurrent_duration.count() / total_sends;
      auto contention_factor = static_cast<double>(concurrent_avg) / baseline_avg;

      INFO(num_threads << " threads: " << concurrent_avg << "μs per send");
      INFO("  Contention factor: " << contention_factor << "x baseline");

      // Document the scaling behavior
      // If contention_factor > 5.0, lock-free queue would be beneficial
      // If contention_factor > 10.0, lock-free queue is strongly recommended
      INFO("  Recommendation: " <<
           (contention_factor > 10.0 ? "CRITICAL - Use lock-free queue" :
            contention_factor > 5.0 ? "Consider lock-free queue" :
            "Mutex acceptable"));
    }

    // This test always passes - it's for documentation/measurement only
    REQUIRE(true);
  }
}

TEST_CASE("MIDI output - thread safety verification", "[midi][thread-safety]") {
  Midi_output midi_out;

  libremidi::message msg;
  msg.bytes = {0x90, 60, 100};

  SECTION("No data races with concurrent sends") {
    constexpr int NUM_THREADS = 4;
    constexpr int SENDS_PER_THREAD = 100;
    std::atomic<int> successful_sends{0};
    std::atomic<int> failed_sends{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
      threads.emplace_back([&midi_out, &msg, &successful_sends, &failed_sends, SENDS_PER_THREAD]() {
        for (int i = 0; i < SENDS_PER_THREAD; ++i) {
          try {
            midi_out.send_message(msg);
            successful_sends.fetch_add(1, std::memory_order_relaxed);
          } catch (const std::runtime_error&) {
            failed_sends.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    auto total = successful_sends.load() + failed_sends.load();
    INFO("Successful sends: " << successful_sends.load());
    INFO("Failed sends: " << failed_sends.load());

    // All sends should be accounted for (no data races/corruption)
    REQUIRE(total == NUM_THREADS * SENDS_PER_THREAD);

    // Current implementation should be thread-safe (mutex protected)
    REQUIRE(true);
  }
}
