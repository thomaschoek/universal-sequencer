#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "utils/atomic_deque.h"
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable Event_t, typename Handler_t>
class Atomic_sequencer {
  using clock = std::chrono::steady_clock;
  typedef clock::time_point steady_time_point;

public:
  explicit Atomic_sequencer(Handler_t handler,
                            atomic_deque::Atomic_deque<Event_t>& seq)
      : event_handler(handler), sequence(seq) {}

  void start(steady_time_point start_time = clock::now());
  void stop();
  bool is_running() const;

private:
  void run(std::stop_token st, steady_time_point start_time);
  Event_t next_event();
  Handler_t event_handler;
  void fire_and_forget(Event_t&& event) const;

  atomic_deque::Atomic_deque<Event_t>& sequence;
  atomic_deque::Atomic_deque<Event_t>::iterator event_itr;

  std::mutex mutex_;
  std::jthread thread_;
};

template <sequencable::Sequencable Event_t, typename Handler_t>
Event_t Atomic_sequencer<Event_t, Handler_t>::next_event() {
  std::scoped_lock{sequence.lock()};
  if (sequence.empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  if (event_itr >= sequence.end() || event_itr < sequence.begin()) {
    event_itr = sequence.begin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *event_itr++;
}

template <sequencable::Sequencable Event_t, typename Handler_t>
inline void
Atomic_sequencer<Event_t, Handler_t>::fire_and_forget(Event_t&& event) const {
  std::ignore = std::async(std::launch::async, event_handler, std::move(event));
}

template <sequencable::Sequencable Event_t, typename Handler_t>
void Atomic_sequencer<Event_t, Handler_t>::run(std::stop_token st,
                                               steady_time_point start_time) {

#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = clock::now();
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
    event_buffer = next_event();

    std::chrono::time_point<clock, std::chrono::duration<double>> event_time =
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
      // Schedule event to handler asynchronously immediately after waking up
      fire_and_forget(std::move(event_buffer));
      // Load next event into buffer
      event_buffer = next_event();

      // Next event should be scheduled after current event completes
      event_time += duration_cache;
    }
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] In Sequencer::run: " << e.what() << std::endl;
    return;
  }
}

template <sequencable::Sequencable Event_t, typename Handler_t>
void Atomic_sequencer<Event_t, Handler_t>::start(
    std::chrono::time_point<clock> start_time) {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};
  if (is_running()) {
    return;
  }

  while (sequence.empty()) {
    // Wait until user adds something to the sequence
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  thread_ = std::jthread(&Atomic_sequencer::run, this, start_time);
}

template <sequencable::Sequencable Event_t, typename Handler_t>
void Atomic_sequencer<Event_t, Handler_t>::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};

  if (is_running()) {
    thread_.request_stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }
}

template <sequencable::Sequencable Event_t, typename Handler_t>
inline bool Atomic_sequencer<Event_t, Handler_t>::is_running() const {
  return thread_.joinable();
}

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
