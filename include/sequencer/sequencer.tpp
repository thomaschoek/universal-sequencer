#include "sequencer.h"
#include <cassert>

namespace Micro_composer {

namespace sequencer {

// PUBLIC
// Constructors

template <Has_duration T_event>
Sequencer<T_event>::Sequencer(Data_init_list data) : events_(data) {}

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
  scheduler_ =
      std::jthread([this, start_time, repeat](std::stop_token stop_token) {
        // Ensure synchronization with other transports: add static
        // min_duration_ and busy_wait_time_ to the actual start time
        if (repeat) {
          this->repeat(stop_token, start_time + min_duration_ + busy_wait_);
        } else {
          this->once(stop_token, start_time + min_duration_ + busy_wait_);
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
                               const size_t reset_pos) {
  pause(reset_time);
  set_next(reset_pos);
}

template <Has_duration T_event>
inline bool Sequencer<T_event>::is_scheduling() const {
  return scheduler_.joinable();
}

// Wait for events to be scheduled by the sequencer and execute handler as they
// arrive
template <Has_duration T_event>
void Sequencer<T_event>::listen(Event_handler handler) {
  while (is_scheduling()) {
    consume();
  }
}

// Synchronize with scheduler thread and consume the scheduled event buffer
template <Has_duration T_event> inline T_event&& Sequencer<T_event>::consume() {
  const Time_point t_next = t_next_.load(std::memory_order_acquire);
  std::this_thread::sleep_until(t_next - busy_wait_);
  while (Clock::now() < t_next) {
    ;
  }
  handler(std::move(buffer_));
}

// Get the time of the next scheduled tick

template <Has_duration T_event>
Sequencer<T_event>::Time_point Sequencer<T_event>::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Time signature CRUD thread-safe operations

template <Has_duration T_event>
inline std::vector<T_event> Sequencer<T_event>::data() const noexcept {
  return events_.data();
}

template <Has_duration T_event> inline bool Sequencer<T_event>::empty() {
  await_scheduler_idle();
  return events_.empty();
}

template <Has_duration T_event> inline size_t Sequencer<T_event>::size() {
  await_scheduler_idle();
  return events_.size();
}

template <Has_duration T_event> void Sequencer<T_event>::set_next(size_t pos) {
  await_scheduler_idle();
  events_.set_next(pos);
}

template <Has_duration T_event>
void Sequencer<T_event>::assign(size_t n, const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  await_scheduler_idle();
  events_.assign(n, event);
}
template <Has_duration T_event>
void Sequencer<T_event>::push_back(const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  events_.push_back(event);
}

template <Has_duration T_event> void Sequencer<T_event>::pop_back() {
  await_scheduler_idle();
  events_.pop_back();
}

template <Has_duration T_event>
void Sequencer<T_event>::insert(size_t pos, const T_event& event) {
  if (static_cast<Duration>(event) < min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  events_.insert(pos, event);
}
template <Has_duration T_event> void Sequencer<T_event>::erase(size_t pos) {
  events_.erase(pos);
}
template <Has_duration T_event>
void Sequencer<T_event>::assign(Data_init_list events) {
  events_.assign(events);
}
template <Has_duration T_event>
void Sequencer<T_event>::assign(const std::vector<T_event>& events) {
  events_.assign(events);
}

template <Has_duration T_event> void Sequencer<T_event>::clear() noexcept {
  await_scheduler_idle();
  events_.clear();
}

// PRIVATE
//
//
template <Has_duration T_event>
inline void Sequencer<T_event>::schedule(const std::stop_token st,
                                         const Time_point t_next,
                                         const T_event& event) {
  // Inform other threads of new time interval start with memory order release
  t_next_.store(t_next, std::memory_order_release);

  buffer_ = event;

  // Wait until approximate time
  std::this_thread::sleep_until(t_next);
}

template <Has_duration T_event>
void Sequencer<T_event>::once(const std::stop_token st,
                              const Time_point initial_tick,
                              const size_t initial_i) {
  if (events_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  events_.set_next(initial_i);
  Time_point t_next = initial_tick;
  Duration event_dur;
  for (size_t i = 0; i < events_.size() && !st.stop_requested(); ++i) {
    const T_event& evt = events_.at(i);
    event_dur = static_cast<Duration>(evt);
    schedule(st, t_next, T_event(evt));
    t_next += event_dur;
  }
}

template <Has_duration T_event>
void Sequencer<T_event>::repeat(const std::stop_token st,
                                const Time_point initial_tick,
                                const size_t initial_i) {
  if (events_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  events_.set_next(initial_i);
  Time_point t_next = initial_tick;
  T_event event;
  Duration event_dur;
  while (!st.stop_requested() && !events_.empty()) {
    event = events_.next();
    event_dur = static_cast<Duration>(event);
    schedule(st, t_next, std::move(event));
    t_next += event_dur;
  }
}

template <Has_duration T_event>
inline void Sequencer<T_event>::await_scheduler_idle() {
  while (is_scheduling() &&
         t_next_.load(std::memory_order_acquire) < Clock::now())
    // If t_next_ is less than now, it means we are in the (presumably very
    // small) time window where either the callback is executing or load_t_next
    // is being called, but t_next_ has not yet been atomically updated with the
    // new value - otherwise it would be greater than Clock::now(). We want
    // load_t_next to be able to acquire the time_sig_ mutex immediately during
    // this period to prevent scheduler lag and inaccuracy.
    // Hence, this is a mechanism to minimize contention on the time_sig_ mutex
    // during that time window. Use this function to ensure that any operation
    // that could contend with the runner thread will execute while the runner
    // thread is sleeping
    std::this_thread::yield();
}

} // namespace sequencer

} // namespace Micro_composer
