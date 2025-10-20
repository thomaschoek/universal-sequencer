#include "sequencer.h"
#include <cassert>

namespace Micro_composer {

namespace sequencer {

// PUBLIC
// Constructors

template <Has_duration T_event>
Sequencer<T_event>::Sequencer(Data_init_list data) : events_(data) {}

template <Has_duration T_event>
Sequencer<T_event>::Sequencer(const std::vector<T_event>& data)
    : events_(data) {}

template <Has_duration T_event>
Sequencer<T_event>::Sequencer(std::vector<T_event>&& data)
    : events_(std::move(data)) {}

template <Has_duration T_event>
Sequencer<T_event>::Sequencer(Sequencer&& other) noexcept
    : events_(other.events_.data()) {
  // Stop the other sequencer if it's running
  if (other.is_scheduling()) {
    other.pause(Clock::now());
  }

  // Copy atomic values (can't be moved)
  t_next_.store(other.t_next_.load(std::memory_order_acquire),
                std::memory_order_release);

  // Note: scheduler_ and transport_mutex_ are default-initialized
  // (stopped/unlocked)
}

// Transport

template <Has_duration T_event>
void Sequencer<T_event>::start(const Time_point start_time, const bool repeat) {
  if (start_time < Clock::now()) {
    throw std::invalid_argument("Start time cannot be in the past!");
  }
  if (is_scheduling()) {
    return;
  }
  std::scoped_lock lock(transport_mutex_);
  scheduler_ = std::jthread([this, start_time,
                             repeat](std::stop_token stop_token) {
    // Ensure synchronization with other transports: add static
    // min_duration_ and busy_wait_time_ to the actual start time
    if (repeat) {
      this->repeat(stop_token,
                   start_time + min_duration_ + busy_wait_duration_);
    } else {
      this->once(stop_token, start_time + min_duration_ + busy_wait_duration_);
    }
  });
}

template <Has_duration T_event>
void Sequencer<T_event>::pause(const Time_point stop_time) {
  if (stop_time < Clock::now()) {
    throw std::invalid_argument("Stop time cannot be in the past!");
  }
  if (!is_scheduling()) {
    return;
  }
  std::scoped_lock lock(transport_mutex_);
  std::this_thread::sleep_until(stop_time);
  scheduler_.request_stop();
  if (scheduler_.joinable()) {
    scheduler_.join();
  }
}

template <Has_duration T_event>
void Sequencer<T_event>::reset(const Time_point reset_time,
                               const Size_type reset_pos) {
  pause(reset_time);
  set_next(reset_pos);
}

template <Has_duration T_event>
inline bool Sequencer<T_event>::is_scheduling() const {
  return scheduler_.joinable();
}

// Get the time of the next scheduled event

template <Has_duration T_event>
Sequencer<T_event>::Time_point Sequencer<T_event>::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Time signature CRUD thread-safe operations

template <Has_duration T_event>
inline std::vector<T_event> Sequencer<T_event>::data() const noexcept {
  std::scoped_lock lck{lock_events()};
  return events_.data();
}

template <Has_duration T_event> inline bool Sequencer<T_event>::empty() {
  std::scoped_lock lck{lock_events()};
  return events_.size() == 0;
}

template <Has_duration T_event>
inline Sequencer<T_event>::Size_type Sequencer<T_event>::size() {
  std::scoped_lock lck{lock_events()};
  return events_.size();
}

template <Has_duration T_event>
void Sequencer<T_event>::set_next(Size_type pos) {
  std::scoped_lock lck{lock_events()};
  events_.set_next(pos);
}

template <Has_duration T_event>
void Sequencer<T_event>::assign(Size_type n, const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  std::scoped_lock lck{lock_events()};
  events_.assign(n, event);
}
template <Has_duration T_event>
void Sequencer<T_event>::push_back(const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  std::scoped_lock lck{lock_events()};
  events_.push_back(event);
}

template <Has_duration T_event> void Sequencer<T_event>::pop_back() {
  std::scoped_lock lck{lock_events()};
  events_.pop_back();
}

template <Has_duration T_event>
void Sequencer<T_event>::insert(Size_type pos, const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  if (pos > events_.size()) {
    throw std::out_of_range("Index out of range!");
  }

  Size_type current{0};
  while (is_scheduling()) {
    current = current_.load(std::memory_order_acquire);
    if (current < pos) {
      break;
    } else if (current > pos + 1) {
      break;
    }
    std::this_thread::yield();
  }

  std::scoped_lock lck{lock_events()};
  events_.insert(pos, event);

  if (current > pos) {
#ifndef NDEBUG
    if (is_scheduling()) {
      assert(current > pos + 1 && "Expected current to be greater than pos + 1 "
                                  "during on-the-fly insert.");
    }
#endif
    if (current < events_.size() - 1) {
      current_.fetch_add(1, std::memory_order_acq_rel);
    } else {
      current_.store(0, std::memory_order_release);
    }
  }
}
template <Has_duration T_event> void Sequencer<T_event>::erase(Size_type idx) {
  if (idx >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }
  Size_type current{0};
  while (is_scheduling()) {
    current = current_.load(std::memory_order_acquire);
    if (current < idx - 1 || current > idx) {
      break;
    }
    std::this_thread::yield();
  }
  std::scoped_lock lck{lock_events()};
  events_.erase(idx);
  if (current > idx) {
    // unsigned Size_type idx >= 0; so current > idx implies current > 0
    current_.fetch_sub(1, std::memory_order_acq_rel);
  }
}
template <Has_duration T_event>
void Sequencer<T_event>::assign(Data_init_list events) {
  std::scoped_lock lck{lock_events()};
  events_.assign(events);
}
template <Has_duration T_event>
void Sequencer<T_event>::assign(const std::vector<T_event>& events) {
  std::scoped_lock lck{lock_events()};
  events_.assign(events);
}

template <Has_duration T_event> void Sequencer<T_event>::clear() noexcept {
  std::scoped_lock lck{lock_events()};
  reset();
  events_.clear();
}

// PROTECTED

template <Has_duration T_event>
void Sequencer<T_event>::await_scheduler_read() const noexcept {
  while (is_scheduling() &&
         scheduler_has_access_.test_and_set(std::memory_order_acquire))
    std::this_thread::yield();
  scheduler_has_access_.clear(std::memory_order_release);
}

template <Has_duration T_event>
std::scoped_lock<std::mutex> Sequencer<T_event>::lock_transport() const {
  await_scheduler_read();
  return std::scoped_lock<std::mutex>(transport_mutex_);
}

template <Has_duration T_event>
std::scoped_lock<std::mutex> Sequencer<T_event>::lock_events() const {
  await_scheduler_read();
  return std::scoped_lock<std::mutex>(data_mutex_);
}

// PRIVATE

template <Has_duration T_event>
Sequencer<T_event>::Time_point
Sequencer<T_event>::once(const std::stop_token st,
                         const Time_point initial_time,
                         const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return initial_time;
  }
  if (initial_time <= Clock::now() + min_duration_ + busy_wait_duration_) {
    throw std::invalid_argument(
        "For synchronization purposes, initial tick must be at least " +
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                           min_duration_ + busy_wait_duration_)
                           .count()) +
        " ms in the future!");
  }

  Size_type events_sze;
  Size_type event_idx = initial_index;
  Time_point next_event_time = initial_time;
  t_next_.store(initial_time, std::memory_order_release);
  current_.store(initial_index, std::memory_order_release);

  do {
    // Inform concurrent threads which event we are about to copy
    event_idx = current_.load(std::memory_order_acquire);
    // Load size of events_ with memory order acquire
    events_sze = events_.size();
    if (events_sze == 0) {
      break;
    } else if (event_idx >= events_sze) {
      event_idx = 0;
    }

    scheduler_has_access_.test_and_set(std::memory_order_release);

    // As other threads may not write to
    // events_[current_.load(memory_order_acquire)], nor modify the events_
    // container as a whole without first safeguarding the integrity of the
    // same, we can now safely copy the shared event into thread local memory
    const T_event evt = events_[event_idx];

    current_.store(event_idx + 1, std::memory_order_release);

    scheduler_has_access_.clear(std::memory_order_release);

    // Set scheduled time on event for output queue consumers
    evt.scheduled_time = next_event_time;

    // Copy event duration to local variable as it will be moved to output queue
    // right after sleep
    const Duration evt_dur = static_cast<Duration>(evt);

    std::this_thread::sleep_until(evt.scheduled_time - busy_wait_duration_);

    if (st.stop_requested()) {
      return evt.scheduled_time;
    }

    // Busy-wait until near exact scheduled time
    while (Clock::now() < evt.scheduled_time) {
      ;
    }
    // Publish event to the output queue
    output_.push(std::move(evt));

    // Update evt_time so next event will schedule right after current's
    // duration ends
    next_event_time += evt_dur;
    t_next_.store(next_event_time, std::memory_order_release);
  } while (!st.stop_requested());

  return next_event_time;
}

template <Has_duration T_event>
void Sequencer<T_event>::repeat(const std::stop_token st,
                                const Time_point initial_time,
                                const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return;
  }
  if (initial_time <= Clock::now() + min_duration_ + busy_wait_duration_) {
    throw std::invalid_argument(
        "For synchronization purposes, initial tick must be at least " +
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                           min_duration_ + busy_wait_duration_)
                           .count()) +
        " ms in the future!");
  }

  Time_point t_next = initial_time;
  Size_type current = initial_index;
  do {
    t_next = once(st, t_next, current);
  } while (!st.stop_requested() && (current = 0) < events_.size());
}

} // namespace sequencer

} // namespace Micro_composer
