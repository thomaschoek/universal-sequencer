#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "utils/atomic_deque.h"
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

namespace MicroComposer {

namespace sequencer {

template <sequencable::Sequencable EVENT_T, typename HandlerT>
class AtomicSequencer {
public:
  explicit AtomicSequencer(HandlerT handler,
                           atomic_deque::AtomicDeque<EVENT_T>& seq)
      : event_handler(handler), sequence(seq) {}

  void start();
  void stop();
  bool is_running() const;

private:
  void run(std::stop_token st);
  EVENT_T next_event();
  HandlerT event_handler;

  atomic_deque::AtomicDeque<EVENT_T>& sequence;
  atomic_deque::AtomicDeque<EVENT_T>::iterator step_itr;

  std::mutex mutex_;
  std::jthread thread_;
};

template <sequencable::Sequencable EVENT_T, typename HandlerT>
EVENT_T AtomicSequencer<EVENT_T, HandlerT>::next_event() {
  std::scoped_lock{sequence.lock()};
  if (sequence.empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  if (step_itr >= sequence.end() || step_itr < sequence.begin()) {
    step_itr = sequence.begin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *step_itr++;
}

template <sequencable::Sequencable EVENT_T, typename HandlerT>
void AtomicSequencer<EVENT_T, HandlerT>::run(std::stop_token st) {

#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] in Sequencer::run at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  try {

    std::optional<EVENT_T> event_buffer;
    event_buffer = next_event();

    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::duration<double>>
        event_time = std::chrono::steady_clock::now();

    std::chrono::duration<double> stored_event_duration =
        event_buffer->duration;

    while (!st.stop_requested()) {
      // Add the current step's offset to trigger time
      event_time += event_buffer->offset;

      // Store this event's duration before it's moved out of scope to handler
      stored_event_duration = event_buffer->duration;

      // IMPORTANT! DO NOT put anything in between the following 3 statements
      // crucial for timing accuracy and to prevent undefined behaviour due to
      // moved out event buffer
      //
      // Sleep until the next trigger time
      std::this_thread::sleep_until(event_time);
      // Move event to handler immediately after waking up
      event_handler(std::move(event_buffer.value()));
      // Load next event into buffer
      event_buffer = next_event();

      // Next event should be scheduled after current event completes
      event_time += stored_event_duration;
    }
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] In Sequencer::run: " << e.what() << std::endl;
    return;
  }
}

template <sequencable::Sequencable EVENT_T, typename HandlerT>
void AtomicSequencer<EVENT_T, HandlerT>::start() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};
  if (thread_.joinable()) {
    // Already running
    return;
  }

  while (sequence.empty()) {
    // Wait until user adds something to the sequence
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  thread_ = std::jthread(&AtomicSequencer::run, this);
}

template <sequencable::Sequencable EVENT_T, typename HandlerT>
void AtomicSequencer<EVENT_T, HandlerT>::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};

  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
}

template <sequencable::Sequencable EVENT_T, typename HandlerT>
inline bool AtomicSequencer<EVENT_T, HandlerT>::is_running() const {
  return thread_.joinable();
}

} // namespace sequencer
} // namespace MicroComposer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
