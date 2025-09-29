#include "sequence/atomic_ring_deque.tpp"
#include "sequencer/atomic_sequencer.h"

#include <exception>
#include <iostream>

namespace Micro_composer {

namespace sequencer {

// PUBLIC:
// Constructors

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(const Atomic_sequencer& other)
    : handler_(other.handler_), steps_(other.steps_) {}

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Atomic_sequencer&& other) noexcept
    : handler_(std::move(other.handler_)), Sequence(std::move(other)) {}

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Handler handler)
    : handler_(handler), Sequence() {}

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Atomic_sequencer(Initializer_list seq,
                                            Handler handler)
    : handler_(handler), Sequence(seq) {}

// Assignment
template <Sequencable Event_t>
Atomic_sequencer<Event_t>&
Atomic_sequencer<Event_t>::operator=(const Atomic_sequencer& other) {
  if (this != &other) {
    std::scoped_lock lck{transport_mutex_};
    handler_ = other.handler_;
    steps_.operator=(other);
  }
  return *this;
}

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

  while (steps_.empty()) {
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

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::set_handler(const Handler handler) {
  std::scoped_lock lck{transport_mutex_};
  handler_ = handler;
}

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::assign(Initializer_list seq) {
  std::scoped_lock lck{transport_mutex_};
  steps_.assign(seq);
}

// CRUD Operations

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::push_back(const Event_t& value) {
  std::scoped_lock lck{transport_mutex_};
  steps_.push_back(value);
}

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::push_back(Event_t&& value) {
  std::scoped_lock lck{transport_mutex_};
  steps_.push_back(std::move(value));
}

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::push_front(const Event_t& value) {
  std::scoped_lock lck{transport_mutex_};
  steps_.push_front(value);
}

template <Sequencable Event_t>
void Atomic_sequencer<Event_t>::push_front(Event_t&& value) {
  std::scoped_lock lck{transport_mutex_};
  steps_.push_front(std::move(value));
}

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Step_iterator
Atomic_sequencer<Event_t>::insert(Step_idx pos, const Event_t& value) {
  std::scoped_lock lck{transport_mutex_};
  return steps_.insert(pos, value);
}

template <Sequencable Event_t>
Atomic_sequencer<Event_t>::Step_iterator
Atomic_sequencer<Event_t>::insert(Step_idx pos, Event_t&& value) {
  std::scoped_lock lck{transport_mutex_};
  return steps_.insert(pos, std::move(value));
}

template <Sequencable Event_t> void Atomic_sequencer<Event_t>::pop_back() {
  std::scoped_lock lck{transport_mutex_};
  steps_.pop_back();
}

template <Sequencable Event_t> void Atomic_sequencer<Event_t>::pop_front() {
  std::scoped_lock lck{transport_mutex_};
  steps_.pop_front();
}

template <Sequencable Event_t> void Atomic_sequencer<Event_t>::clear() {
  std::scoped_lock lck{transport_mutex_};
  steps_.clear();
}

template <Sequencable Event_t>
inline Atomic_sequencer<Event_t>::Step_idx
Atomic_sequencer<Event_t>::size() const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.size();
}

template <Sequencable Event_t>
inline bool Atomic_sequencer<Event_t>::empty() const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.empty();
}

template <Sequencable Event_t>
inline Event_t& Atomic_sequencer<Event_t>::front() {
  std::scoped_lock lck{transport_mutex_};
  return steps_.front();
}

template <Sequencable Event_t>
inline const Event_t& Atomic_sequencer<Event_t>::front() const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.front();
}

template <Sequencable Event_t>
inline Event_t& Atomic_sequencer<Event_t>::back() {
  std::scoped_lock lck{transport_mutex_};
  return steps_.back();
}

template <Sequencable Event_t>
inline const Event_t& Atomic_sequencer<Event_t>::back() const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.back();
}

template <Sequencable Event_t>
inline Event_t& Atomic_sequencer<Event_t>::at(Step_idx pos) {
  std::scoped_lock lck{transport_mutex_};
  return steps_.at(pos);
}

template <Sequencable Event_t>
inline const Event_t& Atomic_sequencer<Event_t>::at(Step_idx pos) const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.at(pos);
}

template <Sequencable Event_t>
inline Event_t& Atomic_sequencer<Event_t>::operator[](Step_idx pos) {
  std::scoped_lock lck{transport_mutex_};
  return steps_.operator[](pos);
}

template <Sequencable Event_t>
inline const Event_t&
Atomic_sequencer<Event_t>::operator[](Step_idx pos) const {
  std::scoped_lock lck{transport_mutex_};
  return steps_.operator[](pos);
}

template <Sequencable Event_t>
inline const Atomic_sequencer<Event_t>::Sequence&
Atomic_sequencer<Event_t>::steps() const {
  std::scoped_lock lck{transport_mutex_};
  return steps_;
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
    event_buffer = steps_.next();

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
      event_buffer = steps_.next();
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
