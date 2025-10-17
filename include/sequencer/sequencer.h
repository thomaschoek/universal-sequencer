#ifndef MICRO_COMPOSER_SEQUENCER_H
#define MICRO_COMPOSER_SEQUENCER_H

#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "container/atomic_ring_vector.h"

namespace Micro_composer {

namespace sequencer {

template <typename T>
concept Has_duration = requires {
  std::convertible_to<typename T::Duration,
                      std::chrono::steady_clock::duration>;
};

template <Has_duration T> class Sequencer {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Container = container::Atomic_ring_vector<T>;
  using Const_iterator = Container::Const_iterator;
  using Data_init_list = std::initializer_list<T>;
  using Callback = std::function<void(T&&)>;

  Sequencer() = default;
  explicit Sequencer(const Callback, Data_init_list = {});

  void schedule(const std::stop_token, const Time_point, T&&);

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
  std::vector<Duration> time_signature() const noexcept;
  std::vector<T> data() const noexcept;
  bool empty();
  size_t size();
  void set_next(size_t = 0);
  void assign(size_t, const T&);
  void push_back(const T&);
  void pop_back();
  void insert(size_t, const T&);
  void erase(size_t);
  void assign(Data_init_list);
  void assign(const std::vector<T>&);
  void clear() noexcept;

protected:
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_{std::chrono::milliseconds(5)};

private:
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
  Container data_;
  std::atomic<Time_point> t_next_;
};

} // namespace sequencer

} // namespace Micro_composer

#endif
