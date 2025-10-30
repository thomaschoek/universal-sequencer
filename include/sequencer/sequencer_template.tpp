#include "sequencer_template.h"
#include <cassert>
#ifndef NDEBUG
#include <iostream>
#include <syncstream>
#endif

namespace Micro_composer {

namespace sequencer {

#ifndef NDEBUG

inline long get_timestamp_ms() {
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
template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Sequencer(Handler handler, Events_initializer data)
    : pool_(handler), events_(data) {}

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Sequencer(Handler handler, const Container& data)
    : pool_(handler), events_(data) {}

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Sequencer(Handler handler, Container&& data)
    : events_(std::move(data)), pool_(handler) {}

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Sequencer(Sequencer&& other) noexcept
    : events_(std::move(other.events_)), pool_(std::move(other.pool_)) {
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

// Destructor
template <sequencable::Mut_seq_event T_event> Sequencer<T_event>::~Sequencer() {
  pause(Clock::now());
}

// State
template <sequencable::Mut_seq_event T_event>
const Sequencer<T_event>::State Sequencer<T_event>::get_state() const {
  return {.is_scheduling = is_scheduling(),
          .next_event_idx = get_pos(),
          .next_scheduled_time = t_next(),
          .size = size(),
          .events = snapshot()};
};

// Transport

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::start(const Time_point start_time, const bool repeat) {
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

template <sequencable::Mut_seq_event T_event>
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

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::stop(const Time_point time, const Size_type pos) {
  pause(time);
  set_pos(pos);
}

template <sequencable::Mut_seq_event T_event>
inline bool Sequencer<T_event>::is_scheduling() const {
  return scheduler_.joinable();
}

template <sequencable::Mut_seq_event T_event>
inline void Sequencer<T_event>::set_handler(const Handler& handler) {
  pool_.set_handler(handler);
}

// Get the time of the next scheduled event

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Time_point Sequencer<T_event>::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Time signature CRUD thread-safe operations

template <sequencable::Mut_seq_event T_event>
inline bool Sequencer<T_event>::empty() const noexcept {
  return events_.empty();
}

template <sequencable::Mut_seq_event T_event>
inline Sequencer<T_event>::Size_type Sequencer<T_event>::size() const noexcept {
  return events_.size();
}

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Size_type inline Sequencer<T_event>::get_pos()
    const noexcept {
  return current_.load(std::memory_order_acquire);
}

template <sequencable::Mut_seq_event T_event>
inline std::vector<T_event> Sequencer<T_event>::snapshot() const noexcept {
  await_scheduler();
  return events_.snapshot();
}

// Setters / Modifiers

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::set_pos(Size_type pos) {
  range_check(pos);
  await_scheduler();
  current_.store(pos, std::memory_order_release);
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::update(Size_type pos, const T_event& other) {
  validate(other);
  range_check(pos);
  await_scheduler();
  events_.mutate(pos, [&other](T_event&& evt) {
    evt.update(other);
    return std::forward<T_event>(evt);
  });
}

template <sequencable::Mut_seq_event T_event>
template <typename... Args>
void Sequencer<T_event>::update(Size_type pos, Args&&... update_args) {
  range_check(pos);
  await_scheduler();
  // Assume validation exists within the event's update method
  events_.mutate(pos, [&update_args...](T_event&& evt) {
    evt.update(std::forward<Args>(update_args)...);
    return std::forward<T_event>(evt);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::push_back(const T_event& event) {
  validate(event);
  await_scheduler();
  events_.push_back(event);
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::insert(Size_type pos, const T_event& event) {
  validate(event);
  range_check(pos);

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

  await_scheduler();
  events_.insert(pos, event);

  if (current > pos) {
    // We need to increment current to account for the inserted event
#ifndef NDEBUG
    if (is_scheduling()) {
      assert(current > pos + 1 && "Expected current to be greater than pos + 1 "
                                  "during on-the-fly insert.");
    }
#endif
    if (++current < events_.size()) {
      current_.fetch_add(1, std::memory_order_acq_rel);
    } else {
      current_.store(0, std::memory_order_release);
    }
  }
}

// Toggling events on/off

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::enable() {
  await_scheduler();
  events_.mutate([](T_event&& event) {
    event.enabled = true;
    return std::forward<T_event>(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::enable(Size_type idx) {
  range_check(idx);
  await_scheduler();
  events_.mutate(idx, [](T_event&& event) {
    event.enabled = true;
    return std::forward<T_event>(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::disable() {
  await_scheduler();
  events_.mutate([](T_event&& event) {
    event.enabled = false;
    return std::forward<T_event>(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::disable(Size_type idx) {
  range_check(idx);
  await_scheduler();
  events_.mutate(idx, [](T_event&& event) {
    event.enabled = false;
    return std::forward<T_event>(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::toggle() {
  await_scheduler();
  events_.mutate([](T_event&& event) {
    event.enabled = !event.enabled;
    return std::forward<T_event>(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::toggle(Size_type idx) {
  range_check(idx);
  await_scheduler();
  events_.mutate(idx, [](T_event&& event) {
    event.enabled = !event.enabled;
    return std::forward<T_event>(event);
  });
}

// Adjusting sequencer tempo

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::adjust_durations(Duration delta) {
  const Duration min_duration = min_duration_;
  events_.mutate([delta, min_duration](T_event&& event) {
    const Duration new_duration = event.duration + delta;
    if (new_duration < min_duration) {
      return std::forward<T_event>(event);
    } else {
      event.set_duration(new_duration);
      return std::forward<T_event>(event);
    }
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::multiply_durations(double factor) {
  if (factor < 0.0) {
    throw std::invalid_argument("Invalid factor: '" + std::to_string(factor) +
                                "'; Tempo factor cannot be negative!");
  }
  debug_msg("Multiplying durations by factor: " + std::to_string(factor));
  await_scheduler();
  const Duration min_dur = min_duration_;
  events_.mutate([min_dur, factor](T_event&& event) {
    // Convert to floating-point duration, multiply, then round and convert back
    const Duration new_duration = std::chrono::duration_cast<Duration>(
        std::chrono::duration_cast<
            std::chrono::duration<double, Duration::period>>(event.duration) *
        factor);
    debug_msg(
        "Old duration: " +
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                           event.duration)
                           .count()) +
        " ms, New duration: " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(new_duration)
                .count()) +
        " ms");
    if (new_duration >= min_dur) {
      event.set_duration(new_duration);
    }
    return std::move(event);
  });
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::mutate(const Mutator& func) {
  await_scheduler();
  events_.mutate(func);
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::mutate(Size_type idx, const Mutator& func) {
  range_check(idx);
  await_scheduler();
  events_.mutate(idx, func);
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::replace(Size_type idx, const T_event& event) {
  validate(event);
  range_check(idx);
  events_.store(idx, event);
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::replace(Size_type start,
                                 const std::vector<T_event>& events) {
  validate(events);
  range_check(start);
  if (start + events.size() > events_.size()) {
    throw std::out_of_range("Replacement exceeds sequencer size: start index " +
                            std::to_string(start) + " + events size " +
                            std::to_string(events.size()) +
                            " > sequencer size " +
                            std::to_string(events_.size()));
  }
  await_scheduler();
  Size_type i = start;
  for (const auto& event : events) {
    events_.store(i++, event);
  }
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::assign(Events_initializer events) {
  for (const auto& event : events) {
    validate(event);
  }
  std::scoped_lock lck{data_mutex_};
  pause();
  events_.assign(events);
  if (current_.load(std::memory_order_acquire) >= events.size()) {
    current_.store(0, std::memory_order_release);
  }
}
template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::assign(const std::vector<T_event>& events) {
  validate(events);
  std::scoped_lock lck{data_mutex_};
  pause();
  events_ = events;
  if (current_.load(std::memory_order_acquire) >= events.size()) {
    current_.store(0, std::memory_order_release);
  }
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::assign(Size_type n, const T_event& event) {
  validate(event);
  // Stop and clear existing events
  std::scoped_lock lck{data_mutex_};
  pause();
  events_.assign(n, event);
  if (current_.load(std::memory_order_acquire) >= n) {
    current_.store(0, std::memory_order_release);
  }
}

// Delete operations

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::pop_back() {
  await_scheduler();
  events_.pop_back();
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::erase(Size_type idx) {
  range_check(idx);
  Size_type current{0};
  while (is_scheduling()) {
    current = current_.load(std::memory_order_acquire);
    if (current != idx) {
      break;
    }
    std::this_thread::yield();
  }
  await_scheduler();
  events_.erase(idx);
  // If current > idx decrement current to account for the removed event
  if (current > idx) {
    // unsigned Size_type idx >= 0; so current > idx implies current > 0
    current_.fetch_sub(1, std::memory_order_acq_rel);
  }
}

template <sequencable::Mut_seq_event T_event> void Sequencer<T_event>::clear() {
  std::scoped_lock lck{data_mutex_};
  stop();
  events_.clear();
}

// PROTECTED
// Validation
template <sequencable::Mut_seq_event T_event>
inline void Sequencer<T_event>::validate(const std::vector<T_event>& events) {
  for (const auto& event : events) {
    validate(event);
  }
}

template <sequencable::Mut_seq_event T_event>
inline void Sequencer<T_event>::validate(const T_event& event) {
  if (event.duration < min_duration_) {
    throw std::invalid_argument(
        "Duration too small: " +
        std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                           event.duration)
                           .count()) +
        " microseconds > " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()));
  }
}

template <sequencable::Mut_seq_event T_event>
inline void Sequencer<T_event>::validate(const Time_point& scheduled_time) {
  if (scheduled_time <= Clock::now() + min_duration_ - spin_duration_) {
    throw std::invalid_argument(
        "Scheduled time must be at least " +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(min_duration_)
                .count()) +
        " ms in the future!");
  }
}

template <sequencable::Mut_seq_event T_event>
inline void Sequencer<T_event>::range_check(Size_type idx) const {
  if (idx >= events_.size()) {
    throw std::out_of_range("Index out of range!");
  }
}

template <sequencable::Mut_seq_event T_event>
inline Sequencer<T_event>::Time_point
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

template <sequencable::Mut_seq_event T_event>
inline std::scoped_lock<std::mutex>
Sequencer<T_event>::lock_events(Time_point& lock_timeout) const {
  lock_timeout = await_scheduler();
  return std::scoped_lock<std::mutex>(data_mutex_);
}

// PRIVATE

template <sequencable::Mut_seq_event T_event>
Sequencer<T_event>::Time_point
Sequencer<T_event>::once(const std::stop_token st,
                         const Time_point initial_time,
                         const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return initial_time;
  }

  Size_type events_size;
  Size_type cur_idx = initial_index;
  Time_point t_next = initial_time;
  T_event current;

  current_.store(initial_index, std::memory_order_release);

  while (!st.stop_requested() &&
         (cur_idx = current_.fetch_add(1, std::memory_order_acq_rel)) <
             events_.size()) {
    {
      std::scoped_lock lck{data_mutex_};
      // debug_msg("oncw(): Acquired data_mutex_");
      // debug_msg("once(): Loaded next_ = " + std::to_string(event_idx) +
      //" from atomic next_");
      // debug_msg("once(): Loaded events_.size() = " +
      // std::to_string(events_size));

      {
        events_.mutate(cur_idx, [&current, t_next](T_event&& evt) {
          evt.scheduled_time = t_next;
          current = evt;
          return std::forward<T_event>(evt);
        });
      }
    }
    // debug_msg("once(): Released data_mutex_");

    const Duration cur_duration = current.duration;

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

    if (current.enabled) {
      pool_.submit(std::forward<T_event>(current));
    }

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
  }

  return t_next;
}

template <sequencable::Mut_seq_event T_event>
void Sequencer<T_event>::repeat(const std::stop_token st,
                                const Time_point initial_time,
                                const Size_type initial_index) {
  if (initial_index >= events_.size()) {
    return;
  }
  validate(initial_time);
  Time_point t_next = initial_time;
  Size_type current = initial_index;
  do {
    t_next = once(st, t_next, current);
  } while (!st.stop_requested() && (current = 0) < events_.size());
}

} // namespace sequencer

} // namespace Micro_composer
