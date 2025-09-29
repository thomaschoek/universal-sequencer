#include "container/atomic_ring_deque.tpp"
#include "sequencer/atomic_sequencer.h"

#include <exception>
#include <iostream>

namespace Micro_composer {

namespace sequencer {

// PUBLIC:
// Constructors

template <Sequencable_updatable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(const Atomic_sequencer& other)
    : handler_(other.handler_), Base_deque(other.steps_) {}

template <Sequencable_updatable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Atomic_sequencer&& other) noexcept
    : handler_(std::move(other.handler_)), Base_deque(std::move(other)) {}

template <Sequencable_updatable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Handler handler)
    : handler_(handler), Base_deque() {}

template <Sequencable_updatable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Initializer_list seq,
                                            Handler handler)
    : handler_(handler), Base_deque(seq) {}

// Assignment
template <Sequencable_updatable Event_t>
Atomic_sequencer<Event_t>&
Atomic_sequencer<Event_t>::operator=(const Atomic_sequencer& other) {
  if (this != &other) {
    std::scoped_lock lck{mutex_};
    handler_ = other.handler_;
    Base_deque::operator=(other);
  }
  return *this;
}

// Control

template <Sequencable_updatable Event_t>
inline bool Atomic_sequencer<Event_t>::is_running() const {
  return runner_thread_.joinable();
}

template <Sequencable_updatable Event_t>
void Atomic_sequencer<Event_t>::start(Time_point start_time) {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = Clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};
  if (is_running()) {
#ifndef NDEBUG
    std::cout << "ALREADY RUNNING BRO!" << std::endl;
#endif
    return;
  }

  while (Base_deque::empty()) {
// Wait until user adds something to the sequence
#ifndef NDEBUG
    std::cout << "I CAN HAZ STEPS? NO! IS EMPTY!" << std::endl;
#endif
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  runner_thread_ = std::jthread(
      [this, start_time](std::stop_token st) { this->run(st, start_time); });
}

template <Sequencable_updatable Event_t>
void Atomic_sequencer<Event_t>::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = Clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif
  std::scoped_lock lck{mutex_};

  if (is_running()) {
    runner_thread_.request_stop();
    if (runner_thread_.joinable()) {
      runner_thread_.join();
    }
  }
}

template <Sequencable_updatable Event_t>
void Atomic_sequencer<Event_t>::set_handler(const Handler handler) {
  std::scoped_lock lck{mutex_};
  handler_ = handler;
}

// PRIVATE:

template <Sequencable_updatable Event_t>
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
    event_buffer = Base_deque::next();

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
      event_buffer = Base_deque::next();
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
