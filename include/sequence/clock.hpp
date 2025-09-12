#ifndef SEQUENCER_CLOCK_H
#define SEQUENCER_CLOCK_H

#ifndef NDEBUG
#include <iostream>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "timing_config.hpp"

namespace MicroComposer {
namespace sequence {

// Ensures a sequence's events are scheduled on time
struct Sequence_clock {
public:
  const timing_capabilities::TimingCapabilities &timing_capabilities =
      timing_capabilities::TimingCapabilities::get_instance();

private:
  // Control variable for clock's run loop
  std::atomic<bool> live{false};

  // Time interval between ticks
  std::atomic<std::chrono::milliseconds> interval{
      std::chrono::milliseconds{1000}};

  // Thread in which the clock runs
  std::jthread thread_;

  // Used to notify clock consumers
  mutable std::condition_variable cond;

  // Mutex used for the condition variable
  mutable std::mutex cond_mutex;

  const std::chrono::nanoseconds busy_wait_time{timing_capabilities.precision *
                                                2};

  void tick() const;

  void run() const {
#ifndef NDEBUG
    // Print debug message about the exact time the clock started
    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    std::cout << "[DEBUG] in Clock::run at " << now_ms.count() << " ms..."
              << std::endl;
#endif

    // Clock's main loop, to be run on this.thread_
    auto next_notify_time = std::chrono::steady_clock::now();
    auto early_wake_time = next_notify_time - busy_wait_time;

    while (is_live()) {
#ifndef NDEBUG
      std::cout << "[DEBUG] Clock tick at "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count()
                << " ms" << std::endl;
#endif

      tick();

      // Wake up just before threads should be notified
      early_wake_time += get_interval();
      std::this_thread::sleep_until(early_wake_time);

      next_notify_time = early_wake_time + busy_wait_time;
      while (std::chrono::steady_clock::now() < next_notify_time) {
        // Busy wait for precise remaining time until next notify
      }
    }
  }

public:
  inline bool is_live() const {
    // Return whether the clock is running
    return live.load(std::memory_order_relaxed);
  }

  inline std::chrono::milliseconds get_interval() const {
    return interval.load(std::memory_order_relaxed);
  }

  void set_interval(const std::chrono::milliseconds &new_interval) {
    // Check that the new interval is above the minimum allowed
    if (new_interval < timing_capabilities.min_interval) {
      throw std::runtime_error(
          "Error: Clock interval too short for system capabilities.");
    }
    // Set the clock's tick interval
    interval.store(new_interval, std::memory_order_relaxed);
  }

  void start() {
#ifndef NDEBUG
    // Print debug message about the exact time the clock started
    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    std::cout << "[DEBUG] In Clock::start at " << now_ms.count() << " ms..."
              << std::endl;
#endif
    // Start the clock in a parallel thread
    if (live) {
      // Already started
      return;
    }
    // Ensure there is no dangling thread
    if (thread_.joinable()) {
      thread_.join();
    }
    // Set control variable
    live.store(true, std::memory_order_relaxed);
    // Start the clock's thread
    thread_ = std::jthread{&Sequence_clock::run, this};
  }
  void stop() {
#ifndef NDEBUG
    // Print debug message about the exact time the clock started
    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    std::cout << "[DEBUG] In Clock::stop at " << now_ms.count() << " ms..."
              << std::endl;
#endif
    // Use live control variable to stop the clock's running thread
    if (live) {
      live.store(false);
    }
    if (thread_.joinable()) {
      thread_.join();
    }
  }
  void wait() const {
    // Wait on the clock's next tick
    std::unique_lock<std::mutex> cond_lock{cond_mutex};
    cond.wait(cond_lock);
  }
};

} // namespace sequence
} // namespace MicroComposer
#endif
