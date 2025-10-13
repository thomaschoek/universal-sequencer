#include "./repeating_scheduler.h"

namespace Micro_composer {

namespace scheduler {

// PUBLIC
// Constructors

Repeating_scheduler::Repeating_scheduler(Callback handler,
                                         Initializer_list durations)
    : schedule_(handler) {
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    intervals_.push_back(dur);
  }
}

void Repeating_scheduler::start(const Time_point start_time) {
  if (start_time < Clock::now()) {
    throw std::invalid_argument("Start time cannot be in the past!");
  }
  if (is_running()) {
    return;
  }
  std::scoped_lock lock(mutex_);
  runner_ = std::jthread([this, start_time](std::stop_token stop_token) {
    // Ensure synchronization with other transports: add static min_duration_
    // and busy_wait_time_ to the actual start time
    this->run(stop_token, start_time + min_duration_ + busy_wait_);
  });
}

void Repeating_scheduler::stop(const Time_point stop_time) {
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

inline bool Repeating_scheduler::is_running() const {
  return runner_.joinable();
}

void Repeating_scheduler::set_handler(const std::function<void()>& handler) {
  std::scoped_lock lock(mutex_);
  schedule_ = handler;
}

// PRIVATE

void Repeating_scheduler::run(std::stop_token st, const Time_point initial_tick,
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
    // Inform other threads of new time interval start with memory order release
    next_tick_.store(t_next, std::memory_order_release);

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
    schedule_();

    // Schedule next tick
    t_next += intervals_.next();
  }
}

} // namespace scheduler

} // namespace Micro_composer
