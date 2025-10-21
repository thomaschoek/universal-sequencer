#include "sequencer/sequencer.h"
#include <cassert>

namespace Micro_composer {

namespace sequencer {

// PUBLIC
// Constructors

Sequencer::Sequencer(Data_init_list data) : events_(data) {}

Sequencer::Sequencer(const Container& data) : events_(data) {}

Sequencer::Sequencer(Container&& data) : events_(std::move(data)) {}

Sequencer::Sequencer(Sequencer&& other) noexcept
    : events_(std::move(other.events_)) {
  // Stop the other sequencer if it's running
  if (other.is_scheduling()) {
    other.pause(Clock::now());
  }

  // Copy atomic values (can't be moved)
  t_next_.store(other.t_next_.load(std::memory_order_acquire),
                std::memory_order_release);
  current_.store(other.current_.load(std::memory_order_acquire),
                 std::memory_order_release);

  // Note: scheduler_ and transport_mutex_ are default-initialized
  // (stopped/unlocked)
}

// Transport

void Sequencer::start(const Time_point start_time, const bool repeat) {
  if (start_time < Clock::now() + min_duration_ - spin_duration_) {
    throw std::invalid_argument("Start time cannot be in the past!");
  }
  if (is_scheduling()) {
    return;
  }
  std::scoped_lock lock(transport_mutex_);
  Size_type start_index = get_pos();
  if (start_index >= events_.size()) {
    start_index = 0;
  }
  scheduler_ = std::jthread(
      [this, start_time, start_index, repeat](std::stop_token stop_token) {
        // Ensure synchronization with other transports: add static
        // min_duration_ and busy_wait_time_ to the actual start time
        if (repeat) {
          this->repeat(stop_token, start_time + min_duration_, start_index);
        } else {
          this->once(stop_token, start_time + min_duration_, start_index);
        }
      });
}

void Sequencer::pause(const Time_point stop_time) {
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

void Sequencer::stop(const Time_point time, const Size_type pos) {
  pause(time);
  set_pos(pos);
}

inline bool Sequencer::is_scheduling() const { return scheduler_.joinable(); }

Sequencer::Event Sequencer::get_current() {
  if (!is_scheduling()) {
    throw std::runtime_error("Sequencer is not scheduling!");
  }
  while (output_.empty()) {
    if (!is_scheduling()) {
      throw std::runtime_error("Sequencer has stopped scheduling!");
    }
    const Time_point t_next = t_next_.load(std::memory_order_acquire);
    if (t_next > Clock::now()) {
      std::this_thread::sleep_until(t_next - spin_duration_);
      while (Clock::now() < t_next) {
        std::this_thread::yield();
      }
    }
  }
  Event evt = std::move(output_.front());
  output_.pop();
  return evt;
}

// Get the time of the next scheduled event

Sequencer::Time_point Sequencer::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Time signature CRUD thread-safe operations

inline Sequencer::Container Sequencer::data() const noexcept {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  return events_;
}

inline bool Sequencer::empty() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  return events_.empty();
}

inline Sequencer::Size_type Sequencer::size() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  return events_.size();
}

void Sequencer::set_pos(Size_type pos) {
  if (pos >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  current_.store(pos, std::memory_order_release);
}

Sequencer::Size_type inline Sequencer::get_pos() const noexcept {
  return current_.load(std::memory_order_acquire);
}

void Sequencer::assign(Size_type n, const Event& event) {
  if (event.duration < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.assign(n, new Event{event});
}
void Sequencer::push_back(const Event& event) {
  if (event.duration < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.push_back(new Event{event});
}

void Sequencer::pop_back() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.pop_back();
}

void Sequencer::insert(Size_type pos, const Event& event) {
  if (event.duration < min_duration_) {
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

  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.insert(events_.begin() + pos, new Event{event});

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
void Sequencer::erase(Size_type idx) {
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
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.erase(events_.begin() + idx);
  if (current > idx) {
    // unsigned Size_type idx >= 0; so current > idx implies current > 0
    current_.fetch_sub(1, std::memory_order_acq_rel);
  }
}
void Sequencer::assign(Data_init_list events) {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_ = events;
}
void Sequencer::assign(const Container& events) {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_ = events;
}

void Sequencer::clear() noexcept {
  stop();
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.clear();
}

// PROTECTED

Sequencer::Time_point Sequencer::await_scheduler() const noexcept {
  // Mechanism to avoid interfering with the scheduler (make other threads run
  // only while scheduler sleeps). Returns the time until which the scheduler
  // will sleep so operations still busy by then can be aborted.
  Time_point scheduler_wakes = Clock::now() + operation_timeout_;
  while (is_scheduling() && (scheduler_wakes = t_next_.load(
                                 std::memory_order_acquire)) <= Clock::now()) {
    std::this_thread::yield();
    if (!is_scheduling()) {
      return Clock::now() + operation_timeout_;
    }
  }
  return scheduler_wakes;
}

std::scoped_lock<std::mutex>
Sequencer::lock_events(Time_point& lock_timeout) const {
  lock_timeout = await_scheduler();
  return std::scoped_lock<std::mutex>(data_mutex_);
}

// PRIVATE

Sequencer::Time_point Sequencer::once(const std::stop_token st,
                                      const Time_point initial_time,
                                      const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return initial_time;
  }
  if (initial_time <= Clock::now() + min_duration_ - spin_duration_) {
    throw std::invalid_argument(
        "For synchronization purposes, initial tick must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms in the future!");
  }

  Size_type events_size;
  Size_type event_idx = initial_index;
  Time_point accumulated_time = initial_time;
  Event cur_event;

  current_.store(initial_index, std::memory_order_release);

  do {
    // Inform concurrent threads which event we are about to copy
    event_idx = current_.load(std::memory_order_acquire);
    // Load size of events_ with memory order acquire
    events_size = events_.size();
    if (event_idx >= events_size) {
      break;
    }

    // As other threads may not write to
    // events_[current_.load(memory_order_acquire)], nor modify the events_
    // container as a whole without first safeguarding the integrity of the
    // same, we can now safely copy the shared event into thread local memory
    cur_event = *events_[event_idx];
    if (*output_[event_idx] != cur_event) {
      output_[event_idx] = events_[event_idx];
    }

    current_.store(event_idx + 1, std::memory_order_release);

    // Set scheduled time on event for output queue consumers
    cur_event.scheduled_time = accumulated_time;

    // Save event duration before it is moved to output_
    const Duration cur_duration = cur_event.duration;

    // Inform other threads until when scheduler will be idle (or at least not
    // critically engaged) Other threads will load t_next_ with
    // memory_order_acquire and only act on shared data if this t_next_ is in
    // the future
    t_next_.store(accumulated_time, std::memory_order_release);

    std::this_thread::sleep_until(cur_event.scheduled_time - spin_duration_);

    while (Clock::now() < cur_event.scheduled_time - (spin_duration_ * 0.5)) {
      // Check for last-moment aborts
      if (st.stop_requested()) {
        return accumulated_time;
      }
    }

    while (Clock::now() < cur_event.scheduled_time) {
      // Spin until the exact scheduled time (disregarding timing inaccuracies
      // due to OS scheduler policies etc. beyond control of this program)
      ;
    }
    // Notify listeners waiting for output event
    output_cv_.notify_one();

    // Update accumulated event time so next event will schedule right after
    // current's duration ends
    accumulated_time += cur_duration;
  } while (!st.stop_requested());

  return accumulated_time;
}

void Sequencer::repeat(const std::stop_token st, const Time_point initial_time,
                       const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return;
  }
  if (initial_time <= Clock::now() + min_duration_ - spin_duration_) {
    throw std::invalid_argument(
        "For synchronization purposes, initial tick must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
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
