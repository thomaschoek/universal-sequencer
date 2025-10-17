#include "sequencer.h"
#include <cassert>

namespace Micro_composer {

namespace sequencer {

// PUBLIC
// Constructors

template <Has_duration T>
Sequencer<T>::Sequencer(const Callback callback, Data_init_list data)
    : callback_(callback), data_(data) {}

// Transport

template <Has_duration T>
void Sequencer<T>::start(const Time_point start_time, const bool repeat) {
  if (start_time < Clock::now()) {
    throw std::invalid_argument("Start time cannot be in the past!");
  }
  if (is_running()) {
    return;
  }
  std::scoped_lock lock(transport_mutex_);
  runner_ =
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

template <Has_duration T> void Sequencer<T>::pause(const Time_point stop_time) {
  if (stop_time < Clock::now()) {
    throw std::invalid_argument("Stop time cannot be in the past!");
  }
  if (!is_running()) {
    return;
  }
  std::scoped_lock lock(transport_mutex_);
  std::this_thread::sleep_until(stop_time);
  runner_.request_stop();
  if (runner_.joinable()) {
    runner_.join();
  }
}

template <Has_duration T>
void Sequencer<T>::reset(const Time_point reset_time, const size_t reset_pos) {
  pause(reset_time);
  set_next(reset_pos);
}

template <Has_duration T> inline bool Sequencer<T>::is_running() const {
  return runner_.joinable();
}

// Get the time of the next scheduled tick

template <Has_duration T>
Sequencer<T>::Time_point Sequencer<T>::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Callback CRUD

template <Has_duration T>
void Sequencer<T>::set_callback(const Callback handler) {
  const auto lock{lock_callback()};
  callback_ = handler;
}

// Time signature CRUD thread-safe operations

template <Has_duration T>
inline std::vector<T> Sequencer<T>::data() const noexcept {
  return data_.data();
}

template <Has_duration T> inline bool Sequencer<T>::empty() {
  await_runner_idle();
  return data_.empty();
}

template <Has_duration T> inline size_t Sequencer<T>::size() {
  await_runner_idle();
  return data_.size();
}

template <Has_duration T> void Sequencer<T>::set_next(size_t pos) {
  await_runner_idle();
  data_.set_next(pos);
}

template <Has_duration T> void Sequencer<T>::assign(size_t n, const T& event) {
  if (event <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  await_runner_idle();
  data_.assign(n, event);
}
template <Has_duration T> void Sequencer<T>::push_back(const T& event) {
  if (event <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  data_.push_back(event);
}

template <Has_duration T> void Sequencer<T>::pop_back() {
  await_runner_idle();
  data_.pop_back();
}

template <Has_duration T>
void Sequencer<T>::insert(size_t pos, const T& event) {
  if (event <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  data_.insert(pos, event);
}
template <Has_duration T> void Sequencer<T>::erase(size_t pos) {
  data_.erase(pos);
}
template <Has_duration T> void Sequencer<T>::assign(Data_init_list events) {
  data_.assign(events);
}
template <Has_duration T>
void Sequencer<T>::assign(const std::vector<T>& events) {
  data_.assign(events);
}

template <Has_duration T> void Sequencer<T>::clear() noexcept {
  await_runner_idle();
  data_.clear();
}

// PRIVATE
//
//
template <Has_duration T>
inline void Sequencer<T>::schedule(const std::stop_token st,
                                   const Time_point t_next) {
  // Inform other threads of new time interval start with memory order release
  t_next_.store(t_next, std::memory_order_release);

  // Wait until approximate time
  std::this_thread::sleep_until(t_next - busy_wait_);

  // Check whether stop was requested at any point during wait
  if (st.stop_requested()) {
    return;
  }

  // Busy-wait until precise time
  while (Clock::now() < t_next)
    ;
  // Call injected code
  callback_();
}

template <Has_duration T>
void Sequencer<T>::once(const std::stop_token st, const Time_point initial_tick,
                        const size_t initial_i) {
  if (data_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  data_.set_next(initial_i);
  Time_point t_next = initial_tick;
  size_t i = 0;
  while (!data_.empty() && ++i < data_.size()) {
    // Schedule next tick
    schedule(st, t_next);
    t_next += data_.next();
  }
}

template <Has_duration T>
void Sequencer<T>::repeat(const std::stop_token st,
                          const Time_point initial_tick,
                          const size_t initial_i) {
  if (data_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  data_.set_next(initial_i);
  Time_point t_next = initial_tick;
  while (!data_.empty()) {
    schedule(st, t_next);
    t_next += data_.next();
  }
}

template <Has_duration T> inline void Sequencer<T>::await_runner_idle() {
  while (is_running() && t_next_.load(std::memory_order_acquire) < Clock::now())
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

template <Has_duration T>
std::scoped_lock<std::mutex> Sequencer<T>::lock_callback() {
  auto t_next = t_next_.load(std::memory_order_acquire);
  if (Clock::now() >= t_next - busy_wait_) {
    std::this_thread::sleep_until(t_next + busy_wait_);
  }

  return std::scoped_lock{callback_mutex_};
}

} // namespace sequencer

} // namespace Micro_composer
