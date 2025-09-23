#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "utils/atomic_deque.h"
#include "utils/event_thread_pool.h"
#include <concepts>
#include <exception>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

namespace MicroComposer {

namespace sequencer {

// Concept for async event handlers
template <typename Handler_t, typename Event_t>
concept AsyncEventHandler = requires(Handler_t handler, Event_t&& event) {
  { handler(std::forward<Event_t>(event)) } -> std::same_as<void>;
} || requires(Handler_t handler, Event_t&& event) {
  { handler(std::forward<Event_t>(event)) } -> std::same_as<std::future<void>>;
};

template <sequencable::Sequencable Event_t,
          AsyncEventHandler<Event_t> Handler_t>
class AtomicSequencer {
public:
  explicit AtomicSequencer(
      Handler_t handler, atomic_deque::AtomicDeque<Event_t>& seq,
      size_t thread_pool_size = std::thread::hardware_concurrency())
      : event_handler(handler), sequence(seq), thread_pool_(thread_pool_size) {}

  void start();
  void stop();
  bool is_running() const;

private:
  void run(std::stop_token st);
  Event_t next_event();
  void schedule_event_handler(Event_t&& event);

  Handler_t event_handler;
  atomic_deque::AtomicDeque<Event_t>& sequence;
  atomic_deque::AtomicDeque<Event_t>::iterator event_itr;
  EventThreadPool thread_pool_;

  std::mutex mutex_;
  std::jthread thread_;
};

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
EVENT_T AtomicSequencer<EVENT_T, HandlerT>::next_event() {
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

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
void AtomicSequencer<EVENT_T, HandlerT>::schedule_event_handler(
    EVENT_T&& event) {
  thread_pool_.submit([this, event = std::move(event)]() mutable {
    event_handler(std::move(event));
  });
}

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
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

      // DO NOT put anything in between the following 3 statements as their
      // immediate succession is crucial for timing accuracy and to prevent
      // undefined behaviour due to moved out event buffer
      //
      // Sleep until the next trigger time
      std::this_thread::sleep_until(event_time);
      // Schedule event to handler asynchronously immediately after waking up
      schedule_event_handler(std::move(event_buffer.value()));
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

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
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

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
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

template <sequencable::Sequencable EVENT_T, AsyncEventHandler<EVENT_T> HandlerT>
inline bool AtomicSequencer<EVENT_T, HandlerT>::is_running() const {
  return thread_.joinable();
}

} // namespace sequencer
} // namespace MicroComposer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
