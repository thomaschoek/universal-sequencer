#ifndef MICRO_COMPOSER_SCHEDULER_H
#define MICRO_COMPOSER_SCHEDULER_H

#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "container/atomic_ring_vector.h"

namespace Micro_composer {

namespace scheduler {

class Scheduler {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Time_signature = container::Atomic_ring_vector<Duration>;
  using Initializer_list = std::initializer_list<Duration>;
  using Callback = std::function<void()>;

  Scheduler() = default;
  explicit Scheduler(Callback handler, Initializer_list durations = {});

  // Thread-safe transport control
  void start(const Time_point start_time = Clock::now(),
             const bool repeat = false);
  void pause(const Time_point pause_time = Clock::now());
  void reset(const Time_point reset_time = Clock::now(),
             const size_t reset_pos = 0);
  bool is_running() const;

  // Set the handler function to be called on each tick
  void set_handler(const std::function<void()>& handler);

  // Thread-safe time signature CRUD operations
  bool empty();
  size_t size();
  void set_pos(size_t = 0);
  void assign(size_t, const Duration&);
  void push_back(const Duration&);
  void insert(size_t, const Duration&);
  void erase(size_t);
  void assign(Initializer_list);
  void assign(const std::vector<Duration>&);

private:
  Duration next(const std::stop_token, const Time_point);
  void once(const std::stop_token,
            const Time_point initial_tick = Clock::now() + min_duration_ +
                                            busy_wait_,
            const size_t initial_i = 0);
  void repeat(const std::stop_token,
              const Time_point initial_tick = Clock::now() + min_duration_ +
                                              busy_wait_,
              const size_t initial_i = 0);
  std::jthread runner_;
  Time_signature intervals_;
  std::atomic<Time_point> next_tick_;
  Callback schedule_ = []() {};
  std::mutex mutex_;
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_{std::chrono::milliseconds(5)};
};

} // namespace scheduler

} // namespace Micro_composer

#endif
