#include "./scheduler.h"

namespace Micro_composer {

namespace scheduler {

// PUBLIC
// Constructors

Scheduler::Scheduler(Callback handler, Initializer_list durations)
    : schedule_(handler) {
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    intervals_.push_back(dur);
  }
}

void Scheduler::start(const Time_point start_time, const bool repeat) {
  if (start_time < Clock::now()) {
    throw std::invalid_argument("Start time cannot be in the past!");
  }
  if (is_running()) {
    return;
  }
  std::scoped_lock lock(mutex_);
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

void Scheduler::stop(const Time_point stop_time) {
  if (stop_time < Clock::now()) {
    throw std::invalid_argument("Stop time cannot be in the past!");
  }
  if (!is_running()) {
    return;
  }
  std::scoped_lock lock(mutex_);
  std::this_thread::sleep_until(stop_time);
  runner_.request_stop();
  if (runner_.joinable()) {
    runner_.join();
  }
}

inline bool Scheduler::is_running() const { return runner_.joinable(); }

void Scheduler::set_handler(const std::function<void()>& handler) {
  std::scoped_lock lock(mutex_);
  schedule_ = handler;
}

// PRIVATE
//
//
inline Scheduler::Duration Scheduler::next(const std::stop_token st,
                                           const Time_point t_next) {
  // Inform other threads of new time interval start with memory order release
  next_tick_.store(t_next, std::memory_order_release);

  // Wait until approximate time
  std::this_thread::sleep_until(t_next - busy_wait_);

  // Check whether stop was requested at any point during wait
  if (st.stop_requested()) {
    return Duration::zero();
  }

  // Busy-wait until precise time
  while (Clock::now() < t_next)
    ;
  // Call injected code
  schedule_();

  // Schedule next tick
  return intervals_.next();
}

void Scheduler::once(const std::stop_token st, const Time_point initial_tick,
                     const size_t initial_i) {
  if (intervals_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  intervals_.set_pos(initial_i);
  Time_point t_next = initial_tick;
  size_t i = 0;
  while (!intervals_.empty() && ++i < intervals_.size()) {
    // Schedule next tick
    t_next += next(st, t_next);
  }
}

void Scheduler::repeat(const std::stop_token st, const Time_point initial_tick,
                       const size_t initial_i) {
  if (intervals_.empty()) {
    return;
  }
  if (initial_tick < Clock::now()) {
    throw std::invalid_argument("Initial tick cannot be in the past!");
  }

  intervals_.set_pos(initial_i);
  Time_point t_next = initial_tick;
  while (!intervals_.empty()) {
    // Schedule next tick
    t_next += next(st, t_next);
  }
}

} // namespace scheduler

} // namespace Micro_composer
