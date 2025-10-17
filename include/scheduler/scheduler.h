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
  using Atomic_dur = std::atomic<Duration>;
  using Time_signature = container::Atomic_ring_vector<Atomic_dur*>;
  using Const_iterator = Time_signature::Const_iterator;
  using Time_sig_init_list = std::initializer_list<Duration>;
  using Callback = std::function<void()>;

  Scheduler() = default;
  explicit Scheduler(const Callback, Time_sig_init_list = {});

  void schedule(const std::stop_token, const Time_point);

  // Thread-safe transport control
  void start(const Time_point start_time = Clock::now(),
             const bool repeat = false);
  void pause(const Time_point pause_time = Clock::now());
  void reset(const Time_point reset_time = Clock::now(),
             const size_t reset_pos = 0);
  bool is_running() const;

  // Get the time of the next scheduled tick
  Time_point t_next() const;

  // Set the handler function to be called on each tick
  void set_callback(const Callback handler);

  // Thread-safe time signature CRUD operations
  bool empty();
  size_t size();
  void set_next(size_t = 0);
  void assign(size_t, const Duration&);
  void push_back(const Duration&);
  void insert(size_t, const Duration&);
  void erase(size_t);
  void assign(Time_sig_init_list);
  void assign(const std::vector<Duration>&);

private:
  Duration load_t_next();
  void once(const std::stop_token,
            const Time_point initial_tick = Clock::now() + min_duration_ +
                                            busy_wait_,
            const size_t initial_i = 0);
  void repeat(const std::stop_token,
              const Time_point initial_tick = Clock::now() + min_duration_ +
                                              busy_wait_,
              const size_t initial_i = 0);

  // The callback that will be scheduled
  Callback callback_ = []() {};

  void await_runner_idle();

  std::scoped_lock<std::mutex> lock_callback();
  mutable std::mutex callback_mutex_;

  mutable std::mutex transport_mutex_;

  std::jthread runner_;
  Time_signature time_sig_;
  std::atomic<Time_point> t_next_;
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_{std::chrono::milliseconds(5)};
};

} // namespace scheduler

} // namespace Micro_composer

#endif
