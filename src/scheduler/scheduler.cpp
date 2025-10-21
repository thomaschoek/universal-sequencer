#include "scheduler/scheduler.h"
#include <cassert>

namespace Micro_composer {

namespace scheduler {

// PUBLIC
// Constructors

Scheduler::Scheduler(const Callback callback, Time_sig_init_list durations)
    : callback_(callback) {
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    time_sig_.push_back(new Atomic_dur{dur});
  }
}

// Transport

void Scheduler::start(const Time_point start_time, const bool repeat) {
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

void Scheduler::pause(const Time_point stop_time) {
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

void Scheduler::reset(const Time_point reset_time, const size_t reset_pos) {
  pause(reset_time);
  set_next(reset_pos);
}

inline bool Scheduler::is_running() const { return runner_.joinable(); }

// Get the time of the next scheduled tick

Scheduler::Time_point Scheduler::t_next() const {
  return t_next_.load(std::memory_order_acquire);
}

// Callback CRUD

void Scheduler::set_callback(const Callback handler) {
  const auto lock{lock_callback()};
  callback_ = handler;
}

// Time signature CRUD thread-safe operations

inline std::vector<Scheduler::Duration>
Scheduler::time_signature() const noexcept {
  std::vector<Atomic_dur*> time_sig = time_sig_.data();
  std::vector<Duration> durations;
  for (const auto& atomic_dur_ptr : time_sig) {
    durations.push_back(atomic_dur_ptr->load(std::memory_order_relaxed));
  }
  return durations;
}

inline bool Scheduler::empty() {
  await_runner_idle();
  return time_sig_.empty();
}

inline size_t Scheduler::size() {
  await_runner_idle();
  return time_sig_.size();
}

void Scheduler::set_next(size_t pos) {
  await_runner_idle();
  time_sig_.set_next(pos);
}

void Scheduler::assign(size_t n, const Duration& dur) {
  if (dur <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  await_runner_idle();
  time_sig_.assign(n, new Atomic_dur{dur});
}
void Scheduler::push_back(const Duration& dur) {
  if (dur <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  time_sig_.push_back(new Atomic_dur{dur});
}

void Scheduler::pop_back() {
  await_runner_idle();
  time_sig_.pop_back();
}

void Scheduler::insert(size_t pos, const Duration& dur) {
  if (dur <= min_duration_) {
    throw std::invalid_argument(std::string(
        "Durations must be at least %lld ms", min_duration_.count()));
  }
  time_sig_.insert(pos, new Atomic_dur{dur});
}
void Scheduler::erase(size_t pos) { time_sig_.erase(pos); }
void Scheduler::assign(Time_sig_init_list durations) {
  std::vector<Atomic_dur*> tmp;
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    tmp.push_back(new Atomic_dur{dur});
  }
  time_sig_.assign(tmp);
}
void Scheduler::assign(const std::vector<Duration>& durations) {
  std::vector<Atomic_dur*> tmp;
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    tmp.push_back(new Atomic_dur{dur});
  }
  await_runner_idle();
  time_sig_.assign(tmp);
}

void Scheduler::clear() noexcept {
  await_runner_idle();
  time_sig_.clear();
}

// PRIVATE
//
//
inline void Scheduler::schedule(const std::stop_token st,
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

inline Scheduler::Duration Scheduler::load_t_next() {
  const Atomic_dur* ptr_to_atomic = time_sig_.next();
  assert(ptr_to_atomic != nullptr);
  Duration t_next = ptr_to_atomic->load(std::memory_order_relaxed);
  return t_next;
}

void Scheduler::once(const std::stop_token st, const Time_point initial_tick,
                     const size_t initial_i) {
  if (time_sig_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  time_sig_.set_next(initial_i);
  Time_point t_next = initial_tick;
  size_t i = 0;
  while (!time_sig_.empty() && ++i < time_sig_.size()) {
    // Schedule next tick
    schedule(st, t_next);
    t_next += load_t_next();
  }
}

void Scheduler::repeat(const std::stop_token st, const Time_point initial_tick,
                       const size_t initial_i) {
  if (time_sig_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  time_sig_.set_next(initial_i);
  Time_point t_next = initial_tick;
  while (!time_sig_.empty()) {
    schedule(st, t_next);
    t_next += load_t_next();
  }
}

inline void Scheduler::await_runner_idle() {
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

std::scoped_lock<std::mutex> Scheduler::lock_callback() {
  auto t_next = t_next_.load(std::memory_order_acquire);
  if (Clock::now() >= t_next - busy_wait_) {
    std::this_thread::sleep_until(t_next + busy_wait_);
  }

  return std::scoped_lock{callback_mutex_};
}

} // namespace scheduler

} // namespace Micro_composer
