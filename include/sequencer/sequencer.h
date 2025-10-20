#ifndef MICRO_COMPOSER_SEQUENCER_H
#define MICRO_COMPOSER_SEQUENCER_H

#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "container/atomic_deque.h"
#include "container/atomic_ring_vector.h"

namespace Micro_composer {

namespace sequencer {

template <typename T>
concept Has_duration = requires {
  {
    T::scheduled_time
  } -> std::convertible_to<std::chrono::steady_clock::time_point>;
  requires std::convertible_to<T, std::chrono::steady_clock::duration>;
};

template <Has_duration T_event> class Sequencer {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Container = container::Atomic_ring_vector<T_event>;
  using Size_type = Container::Size_type;
  using Output_queue = container::Atomic_deque<T_event>;
  using Const_iterator = Container::Const_iterator;
  using Data_init_list = std::initializer_list<T_event>;
  using Event_handler = std::function<void(T_event&&)>;

  explicit Sequencer(Data_init_list = {});
  explicit Sequencer(const std::vector<T_event>&);
  explicit Sequencer(std::vector<T_event>&&);
  Sequencer(Sequencer&&) noexcept;

  void schedule(const std::stop_token, const Time_point, const T_event&);

  const Output_queue& output() const noexcept;

  // Thread-safe transport control
  void start(const Time_point start_time = Clock::now(),
             const bool repeat = false);
  void pause(const Time_point pause_time = Clock::now());
  void reset(const Time_point reset_time = Clock::now(),
             const size_t reset_pos = 0);
  bool is_scheduling() const;

  // Get the time of the next scheduled event
  Time_point t_next() const;

  // Thread-safe time signature CRUD operations
  std::vector<Duration> time_signature() const noexcept;
  std::vector<T_event> data() const noexcept;
  bool empty();
  size_t size();
  void set_next(Size_type = 0);
  void assign(size_t, const T_event&);
  void push_back(const T_event&);
  void pop_back();
  void insert(Size_type, const T_event&);
  void erase(Size_type);
  void assign(Data_init_list);
  void assign(const std::vector<T_event>&);
  void clear() noexcept;

  // Wait until outside of window where scheduler is loading events_[current_]
  void dodge_scheduler(const Size_type) const noexcept;

protected:
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_duration_{
      std::chrono::milliseconds(5)};

private:
  Time_point once(const std::stop_token,
                  const Time_point initial_time = Clock::now() + min_duration_,
                  const Size_type initial_index = 0);
  void repeat(const std::stop_token,
              const Time_point initial_time = Clock::now() + min_duration_,
              const Size_type initial_index = 0);

  void await_scheduler_idle();

  mutable std::mutex transport_mutex_;

  std::jthread scheduler_;
  Container events_;
  std::atomic<Size_type> current_{0};
  Output_queue output_;
  std::atomic<Time_point> t_next_;
};

} // namespace sequencer

} // namespace Micro_composer

#include "sequencer.tpp"

#endif
