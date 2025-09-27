#include "sequence/atomic_ring_deque.tpp"
#include "sequencer/atomic_sequencer.h"

#include <exception>
#include <iostream>

namespace Micro_composer {

namespace sequencer {

// PUBLIC:
// Control

template <Sequencable Event_t>
inline bool Atomic_sequencer<Event_t>::is_running() const {
  return runner_thread_.joinable();
}

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::start(Time_point start_time) {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = Clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{transport_mutex_};
  if (is_running()) {
    return;
  }

  while (Base_t::empty()) {
    // Wait until user adds something to the sequence
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  runner_thread_ = std::jthread(
      [this, start_time](std::stop_token st) { this->run(st, start_time); });
}

template <Sequencable Event_t> void Atomic_sequencer<Event_t>::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = Clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{transport_mutex_};

  if (is_running()) {
    runner_thread_.request_stop();
    if (runner_thread_.joinable()) {
      runner_thread_.join();
    }
  }
}

// PRIVATE:

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::run(std::stop_token st, Time_point start_time) {

#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = Clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] in Sequencer::run at " << now_ms.count() << " ms..."
            << std::endl
            << "[DEBUG] thread " << std::this_thread::get_id()
            << " got start_time "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   start_time.time_since_epoch())
            << " since epoch." << std::endl;
#endif

  try {

    Event_t event_buffer;
    event_buffer = Base_t::next();

    std::chrono::time_point<Clock, std::chrono::duration<double>> event_time =
        start_time;

    std::chrono::duration<double> duration_cache = event_buffer.duration;

    while (!st.stop_requested()) {
      // Add the current step's offset to trigger time
      event_time += event_buffer.offset;
      // Store this event's duration before it's moved out of scope to handler
      duration_cache = event_buffer.duration;
      // DO NOT put anything in between the following 3 statements as their
      // immediate succession is crucial for timing accuracy and to prevent
      // undefined behaviour due to moved out event buffer
      //
      // Sleep until the next trigger time
      std::this_thread::sleep_until(event_time);
      // Move current event buffer to event handler
      handler_(std::move(event_buffer));
      // Load next event into buffer
      event_buffer = Base_t::next();
      // Next event should be scheduled after current event completes
      event_time += duration_cache;
    }
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] In Sequencer::run: " << e.what() << std::endl;
    return;
  }
}

} // namespace sequencer
} // namespace Micro_composer
