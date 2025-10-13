#include "scheduler/scheduler.h"
#include <stdexcept>

namespace Micro_composer {

namespace scheduler {

// Constructor
Scheduler::Scheduler(Callback handler, Initializer_list durations)
    : schedule_(handler), intervals_(durations) {}

// Thread-safe transport control
void Scheduler::start(const Time_point start_time, const bool repeat) {
  std::scoped_lock lck{mutex_};
  if (runner_.joinable()) {
    runner_.request_stop();
    runner_.join();
  }

  if (repeat) {
    runner_ = std::jthread([this](std::stop_token st, Time_point initial_tick) {
      this->repeat(st, initial_tick);
    }, start_time);
  } else {
    runner_ = std::jthread([this](std::stop_token st, Time_point initial_tick) {
      this->once(st, initial_tick);
    }, start_time);
  }
}

void Scheduler::pause(const Time_point pause_time) {
  std::scoped_lock lck{mutex_};
  if (runner_.joinable()) {
    runner_.request_stop();
    runner_.join();
  }
}

void Scheduler::reset(const Time_point reset_time, const size_t reset_pos) {
  std::scoped_lock lck{mutex_};
  if (runner_.joinable()) {
    runner_.request_stop();
    runner_.join();
  }
  intervals_.set_pos(reset_pos);
  next_tick_ = reset_time;
}

bool Scheduler::is_running() const {
  std::scoped_lock lck{mutex_};
  return runner_.joinable() && !runner_.get_stop_token().stop_requested();
}

// Set handler
void Scheduler::set_handler(const std::function<void()>& handler) {
  std::scoped_lock lck{mutex_};
  schedule_ = handler;
}

// Thread-safe time signature CRUD operations
bool Scheduler::empty() {
  return intervals_.empty();
}

size_t Scheduler::size() {
  return intervals_.size();
}

void Scheduler::set_pos(size_t pos) {
  intervals_.set_pos(pos);
}

void Scheduler::assign(size_t pos, const Duration& duration) {
  // This updates a specific position in the time signature
  intervals_.replace(pos, duration);
}

void Scheduler::push_back(const Duration& duration) {
  intervals_.push_back(duration);
}

void Scheduler::insert(size_t pos, const Duration& duration) {
  intervals_.insert(pos, duration);
}

void Scheduler::erase(size_t pos) {
  intervals_.erase(pos);
}

void Scheduler::assign(Initializer_list durations) {
  intervals_.assign(durations);
}

void Scheduler::assign(const std::vector<Duration>& durations) {
  intervals_.assign(durations);
}

// Private methods
Scheduler::Duration Scheduler::next(const std::stop_token st, const Time_point current_time) {
  if (st.stop_requested() || intervals_.empty()) {
    return Duration::zero();
  }
  return intervals_.next();
}

void Scheduler::once(const std::stop_token st, const Time_point initial_tick, const size_t initial_i) {
  next_tick_ = initial_tick;
  intervals_.set_pos(initial_i);

  while (!st.stop_requested() && !intervals_.empty()) {
    // Get next interval duration
    Duration interval = next(st, next_tick_.load());
    if (interval == Duration::zero()) {
      break;
    }

    // Calculate next tick time
    next_tick_ = next_tick_.load() + interval;

    // Sleep until next tick (with busy wait for precision)
    auto sleep_until = next_tick_.load() - busy_wait_;
    std::this_thread::sleep_until(sleep_until);

    // Busy wait for remaining time
    while (Clock::now() < next_tick_.load() && !st.stop_requested()) {
      // Spin
    }

    // Execute callback
    if (!st.stop_requested()) {
      schedule_();
    }

    // Check if we've reached the end
    if (intervals_.get_pos() == 0) {
      // We've wrapped around, stop for "once" mode
      break;
    }
  }
}

void Scheduler::repeat(const std::stop_token st, const Time_point initial_tick, const size_t initial_i) {
  next_tick_ = initial_tick;
  intervals_.set_pos(initial_i);

  while (!st.stop_requested() && !intervals_.empty()) {
    // Get next interval duration
    Duration interval = next(st, next_tick_.load());
    if (interval == Duration::zero()) {
      break;
    }

    // Calculate next tick time
    next_tick_ = next_tick_.load() + interval;

    // Sleep until next tick (with busy wait for precision)
    auto sleep_until = next_tick_.load() - busy_wait_;
    std::this_thread::sleep_until(sleep_until);

    // Busy wait for remaining time
    while (Clock::now() < next_tick_.load() && !st.stop_requested()) {
      // Spin
    }

    // Execute callback
    if (!st.stop_requested()) {
      schedule_();
    }
  }
}

} // namespace scheduler

} // namespace Micro_composer
