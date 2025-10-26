#include "sequencer_template.h"
#include <cassert>
#ifndef NDEBUG
#include <iostream>
#include <syncstream>
#endif

namespace Micro_composer {

namespace sequencer {

#ifndef NDEBUG

long get_timestamp_ms() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

inline void debug_msg(std::string msg, std::ostream& stream = std::cerr) {
#ifndef NDEBUG

  std::osyncstream(stream) << get_timestamp_ms() << " [SEQUENCER] thread "
                           << std::to_string(std::hash<std::thread::id>{}(
                                  std::this_thread::get_id()))
                           << ": " << msg << std::endl
                           << std::flush;
#endif
}

#endif

// PUBLIC
// Constructors
template <sequencable::Sequencable T_event>
Sequencer<T_event>::Sequencer(Handler handler, Events_initializer data)
    : pool_(handler) {
  events_.reserve(data.size());
  for (const auto& item : data) {
    events_.push_back(std::make_unique<T_event>(item));
  }
}

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Sequencer(Handler handler, const Container& data)
    : pool_(handler) {
  events_.reserve(data.size());
  for (const auto& item_ptr : data) {
    events_.push_back(std::make_unique<T_event>(*item_ptr));
  }
}

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Sequencer(Handler handler, Container&& data)
    : events_(std::move(data)), pool_(handler) {}

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Sequencer(Sequencer&& other) noexcept
    : events_(std::move(other.events_)), pool_(std::move(other.pool_)) {
  // Stop the other sequencer if it's running
  if (other.is_scheduling()) {
    other.pause(Clock::now());
  }

  // Copy atomic values (can't be moved)
  t_next_.store(other.t_next_.load(std::memory_order_acquire),
                std::memory_order_release);
  next_.store(other.next_.load(std::memory_order_acquire),
              std::memory_order_release);

  // Note: scheduler_ and transport_mutex_ are default-initialized
  // (stopped/unlocked)
}

// Destructor
template <sequencable::Sequencable T_event> Sequencer<T_event>::~Sequencer() {
  pause(Clock::now());
}

// Transport

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::start(const Time_point start_time, const bool repeat) {
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

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::pause(const Time_point stop_time) {
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

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::stop(const Time_point time, const Size_type pos) {
  pause(time);
  set_pos(pos);
}

template <sequencable::Sequencable T_event>
inline bool Sequencer<T_event>::is_scheduling() const {
  return scheduler_.joinable();
}

template <sequencable::Sequencable T_event>
inline void Sequencer<T_event>::set_handler(const Handler& handler) {
  pool_.set_handler(handler);
}

// Get the time of the next scheduled event

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Time_point Sequencer<T_event>::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Time signature CRUD thread-safe operations

template <sequencable::Sequencable T_event>
inline std::vector<T_event> Sequencer<T_event>::data() const noexcept {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  std::vector<T_event> result;
  result.reserve(events_.size());
  for (const auto& ptr : events_) {
    result.push_back(*ptr);
  }
  return result;
}

template <sequencable::Sequencable T_event>
inline bool Sequencer<T_event>::empty() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  return events_.empty();
}

template <sequencable::Sequencable T_event>
inline Sequencer<T_event>::Size_type Sequencer<T_event>::size() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  return events_.size();
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::set_pos(Size_type pos) {
  if (pos > 0 && pos >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  next_.store(pos, std::memory_order_release);
}

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Size_type inline Sequencer<T_event>::get_pos()
    const noexcept {
  return next_.load(std::memory_order_acquire);
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::assign(Size_type n, const T_event& event) {
  if (event.duration < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  // Stop and clear existing events
  clear();
  std::scoped_lock lck{data_mutex_};
  events_.reserve(n);
  for (Size_type i = 0; i < n; ++i) {
    events_.push_back(std::make_unique<T_event>(event));
  }
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::push_back(const T_event& event) {
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
  events_.push_back(std::make_unique<T_event>(event));
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::pop_back() {
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.pop_back();
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::insert(Size_type pos, const T_event& event) {
  if (event.duration < min_duration_) {
    throw std::invalid_argument(
        "Durations must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms");
  }
  if (pos >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }

  Size_type current;
  while (is_scheduling()) {
    current = next_.load(std::memory_order_acquire);
    if (current < pos) {
      break;
    } else if (current > pos + 1) {
      break;
    }
    std::this_thread::yield();
  }

  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.insert(events_.begin() + pos, std::make_unique<T_event>(event));

  if (current > pos) {
    // We need to increment current to account for the inserted event
#ifndef NDEBUG
    if (is_scheduling()) {
      assert(current > pos + 1 && "Expected current to be greater than pos + 1 "
                                  "during on-the-fly insert.");
    }
#endif
    if (++current < events_.size()) {
      next_.fetch_add(1, std::memory_order_acq_rel);
    } else {
      next_.store(0, std::memory_order_release);
    }
  }
}
template <sequencable::Sequencable T_event>
void Sequencer<T_event>::erase(Size_type idx) {
  if (idx >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }
  Size_type current;
  while (is_scheduling()) {
    current = next_.load(std::memory_order_acquire);
    if (current != idx) {
      break;
    }
    std::this_thread::yield();
  }
  Time_point timeout;
  std::scoped_lock lck{lock_events(timeout)};
  events_.erase(events_.begin() + idx);
  // If current > idx decrement current to account for the removed event
  if (current > idx) {
    // unsigned Size_type idx >= 0; so current > idx implies current > 0
    next_.fetch_sub(1, std::memory_order_acq_rel);
  }
}
template <sequencable::Sequencable T_event>
void Sequencer<T_event>::assign(Events_initializer events) {
  clear();
  std::scoped_lock lck{data_mutex_};
  events_.reserve(events.size());
  for (const auto& item : events) {
    events_.push_back(std::make_unique<T_event>(item));
  }
}
template <sequencable::Sequencable T_event>
void Sequencer<T_event>::assign(const Container& events) {
  debug_msg("entered Sequencer::assign(const Container&)");
  clear();
  debug_msg("returned from clear()");
  std::scoped_lock lck{data_mutex_};
  debug_msg("acquired data_mutex_");
  events_.reserve(events.size());
  debug_msg("reserved events.size()=" + std::to_string(events.size()));
  for (const std::unique_ptr<T_event>& ptr : events) {
    T_event item = *ptr.get();
    debug_msg("pushing back item from pointer " +
              std::to_string(reinterpret_cast<std::uintptr_t>(ptr.get())));
    events_.push_back(std::make_unique<T_event>(std::move(item)));
  }
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::assign(const std::vector<T_event>& events) {
  clear();
  std::scoped_lock lck{data_mutex_};
  events_.reserve(events.size());
  for (const auto& item : events) {
    events_.push_back(std::make_unique<T_event>(item));
  }
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::clear() noexcept {
  stop();
  std::scoped_lock lck{data_mutex_};
  events_.clear();
}

// PROTECTED

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Time_point
Sequencer<T_event>::await_scheduler() const noexcept {
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

template <sequencable::Sequencable T_event>
std::scoped_lock<std::mutex>
Sequencer<T_event>::lock_events(Time_point& lock_timeout) const {
  lock_timeout = await_scheduler();
  return std::scoped_lock<std::mutex>(data_mutex_);
}

// PRIVATE

template <sequencable::Sequencable T_event>
Sequencer<T_event>::Time_point
Sequencer<T_event>::once(const std::stop_token st,
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
  Time_point t_next = initial_time;
  T_event buffer;

  next_.store(initial_index, std::memory_order_release);

  do {
    {
      std::scoped_lock lck{data_mutex_};
      // debug_msg("oncw(): Acquired data_mutex_");
      //  Inform concurrent threads which event we are about to copy
      event_idx = next_.load(std::memory_order_acquire);
      // debug_msg("once(): Loaded next_ = " + std::to_string(event_idx) +
      //" from atomic next_");
      events_size = events_.size();
      // debug_msg("once(): Loaded events_.size() = " +
      // std::to_string(events_size));
      if (event_idx >= events_size) {
        // debug_msg("once(): event_idx " + std::to_string(event_idx)
        // +
        //">= events_size " + std::to_string(events_size) +
        //", BREAK loop");
        break;
      }

      {
        T_event* const cur_ptr = events_[event_idx].get();
        cur_ptr->scheduled_time = t_next;
        buffer = *cur_ptr;
      }
      next_.store(event_idx + 1, std::memory_order_release);
    }
    // debug_msg("once(): Released data_mutex_");

    const Duration cur_duration = buffer.duration;

    // debug_msg(
    //        "once(): Submitting event to pool with (scheduled_time=" +
    //        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
    //                           buffer.scheduled_time.time_since_epoch())
    //                           .count()) +
    //        " ms, duration=" +
    //        std::to_string(
    //            std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration)
    //                .count()) +
    //        " ms)");

    if (st.stop_requested()) {
      debug_msg("once(): stop requested, breaking loop");
      break;
    }
    pool_.submit(std::forward<T_event>(buffer));

    // Inform other threads until when scheduler will be idle (or at least not
    // critically engaged) Other threads will load t_next_ with
    // memory_order_acquire and only try to lock the events mutex during this
    // time window
    // debug_msg(
    //        "once(): Storing t_next_ as " +
    //        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
    //                           t_next.time_since_epoch())
    //                           .count()) +
    //        " ms");
    t_next_.store(t_next, std::memory_order_release);

    std::this_thread::sleep_until(t_next);

    t_next += cur_duration;
  } while (!st.stop_requested());

  return t_next;
}

template <sequencable::Sequencable T_event>
void Sequencer<T_event>::repeat(const std::stop_token st,
                                const Time_point initial_time,
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
