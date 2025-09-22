#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "utils/atomic_deque.h"
#include <atomic>
#include <mutex>
#include <thread>
#ifndef NDEBUG
#include <iostream>
#endif
#include <optional>
#include <functional>

namespace MicroComposer {

namespace sequencer {

template <typename T>
concept Sequencable = requires(T t) {
  { t.duration } -> std::convertible_to<std::chrono::duration<double>>;
  { t.offset } -> std::convertible_to<std::chrono::duration<double>>;
};

template <Sequencable EVENT_T> class AtomicSequencer {

  std::mutex mutex_;
  std::jthread thread_;
  std::atomic<bool> live{false};

  atomic_deque::AtomicDeque<EVENT_T> &sequence;
  atomic_deque::AtomicDeque<EVENT_T>::iterator step_itr;
  std::optional<EVENT_T> next_event();

  void (&event_handler)(const EVENT_T &item);
  void run();

public:
  bool is_live() const { return live.load(std::memory_order_relaxed); };
  void store_live(const bool &val) {
    live.store(val, std::memory_order_relaxed);
  };

  void start();
  void stop();

  explicit AtomicSequencer(void (&handler)(const EVENT_T &item),
                           atomic_deque::AtomicDeque<EVENT_T> &seq)
      : event_handler(handler), sequence(seq) {}
};

template <Sequencable EVENT_T>
std::optional<EVENT_T> AtomicSequencer<EVENT_T>::next_event() {
  std::scoped_lock{sequence.lock()};
  if (sequence.empty()) {
    return std::nullopt;
  }
  if (step_itr >= sequence.end() || step_itr < sequence.begin()) {
    step_itr = sequence.begin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *step_itr++;
}

template <Sequencable EVENT_T> void AtomicSequencer<EVENT_T>::run() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] in Sequencer::run at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  std::optional<EVENT_T> event_buffer;
  if (sequence.empty()) {
    store_live(false);
    return;
  }
  event_buffer = next_event();

  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::duration<double>>
      event_time = std::chrono::steady_clock::now();

  std::chrono::duration<double> stored_event_duration = event_buffer->duration;

  while (is_live()) {
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
    // ...
    // PROFIT!!!

    // Check that buffer has value, else stop running (this means the sequence
    // has been emptied)
    if (!event_buffer.has_value()) {
      store_live(false);
      return;
    }

    // Next event should be scheduled after current event completes
    event_time += stored_event_duration;
  }
}

template <Sequencable EVENT_T> void AtomicSequencer<EVENT_T>::start() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  if (is_live()) {
    return;
  }
  store_live(true);
  thread_ = std::jthread(&AtomicSequencer::run, this);
}

template <Sequencable EVENT_T> void AtomicSequencer<EVENT_T>::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  if (is_live()) {
    store_live(false);
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

} // namespace sequencer
} // namespace MicroComposer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
