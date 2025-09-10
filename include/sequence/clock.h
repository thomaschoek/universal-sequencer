#ifndef SEQUENCER_CLOCK_H
#define SEQUENCER_CLOCK_H

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace MicroComposer_sequence {

// Ensures a sequence's events are scheduled on time
class Sequence_clock {

  // Time to busy wait before notifying threads
  // 30 milliseconds
  static constexpr const std::chrono::microseconds busy_wait_time{
      std::chrono::microseconds(500)};

  // Control variable for clock's run loop
  std::atomic<bool> live{false};

  // Time interval between ticks
  std::atomic<std::chrono::milliseconds> interval{
      std::chrono::milliseconds{1000}};

  // Thread in which the clock runs
  std::thread thread_;

  // Used to notify clock consumers
  mutable std::condition_variable cond;

  // Mutex used for the condition variable
  mutable std::mutex cond_mutex;

  void run() const {
    // Clock's main loop, to be run on this.thread_
    auto next_notify_time = std::chrono::high_resolution_clock::now();
    auto early_wake_time = next_notify_time - busy_wait_time;

    while (is_live()) {
      cond.notify_all();

      // Wake up just before threads should be notified
      early_wake_time += get_interval();
      std::this_thread::sleep_until(early_wake_time);

      next_notify_time = early_wake_time + busy_wait_time;
      while (std::chrono::high_resolution_clock::now() < next_notify_time) {
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
    // Set the clock's tick interval
    interval.store(new_interval, std::memory_order_relaxed);
  }

  void start() {
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
    thread_ = std::thread{&Sequence_clock::run, this};
  }
  void stop() {
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

} // namespace MicroComposer_sequence
#endif
