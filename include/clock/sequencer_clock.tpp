#include "./sequencer_clock.h"

namespace Micro_composer {

namespace sequencer {

namespace transport {

// PUBLIC
// Constructors

Sequencer_transport::Sequencer_transport(Handler handler,
                                         Initializer_list durations)
    : handler_(handler) {
  for (const auto& dur : durations) {
    if (dur <= min_duration_) {
      throw std::invalid_argument(std::string(
          "Durations must be at least %lld ms", min_duration_.count()));
    }
    intervals_.push_back(dur);
  }
}

void Sequencer_transport::start(const Time_point start_time) {
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

void Sequencer_transport::stop(const Time_point stop_time) {
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

inline bool Sequencer_transport::is_running() const {
  return runner_.joinable();
}

void Sequencer_transport::set_handler(const std::function<void()>& handler) {
  std::scoped_lock lock(mutex_);
  handler_ = handler;
}

// PRIVATE

void Sequencer_transport::run(std::stop_token st, const Time_point initial_tick,
                              const Size_type initial_i) {
  {
    // Validate preconditions under lock
    std::scoped_lock lock(mutex_);
    if (intervals_.empty()) {
      return;
    }
    if (initial_i >= intervals_.size()) {
      throw std::out_of_range(
          "Initial index out of range of intervals container");
    }
    if (initial_tick < Clock::now()) {
      throw std::invalid_argument("Initial tick cannot be in the past!");
    }
  }
  Time_point next_tick = initial_tick;
  Iterator i_interval = Iterator{intervals_.begin() + initial_i};
  while (!intervals_.empty()) {
    while (i_interval < intervals_.end()) {
      // Inform other threads when next tick expected
      next_tick_.store(next_tick, std::memory_order_release);

      // Wait until approximate time
      std::this_thread::sleep_until(next_tick - busy_wait_);

      // Check whether stop was requested
      if (st.stop_requested()) {
        return;
      }

      // Busy-wait until precise time
      while (Clock::now() < next_tick)
        ;

      // Handle tick event
      handler_();

      // Schedule next tick
      next_tick += *i_interval;
      // Inform other threads we are done with the interval pointed to by the
      // shared (atomic) interval_itr_ with memory order release. Any thread
      // other than this one should call
      // interval_itr_.load(std::memory_order_acquire) BEFORE doing so.
      interval_itr_.store(i_interval++, std::memory_order_release);
    }
    // Reset interval iterator to beginning of intervals container
    i_interval = intervals_.begin();
  }
}

} // namespace transport

} // namespace sequencer

} // namespace Micro_composer
