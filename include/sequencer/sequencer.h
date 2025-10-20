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
  requires std::convertible_to<T, std::chrono::steady_clock::duration>;
};

template <Has_duration T_event> class Sequencer {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Container = container::Atomic_ring_vector<T_event>;
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

  void listen(Event_handler);
  // Get the time of the next scheduled event
  Time_point t_next() const;

  // Thread-safe time signature CRUD operations
  std::vector<Duration> time_signature() const noexcept;
  std::vector<T_event> data() const noexcept;
  bool empty();
  size_t size();
  void set_next(size_t = 0);
  void assign(size_t, const T_event&);
  void push_back(const T_event&);
  void pop_back();
  void insert(size_t, const T_event&);
  void erase(size_t);
  void assign(Data_init_list);
  void assign(const std::vector<T_event>&);
  void clear() noexcept;

protected:
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_{std::chrono::milliseconds(5)};

  // Consume the event buffer
  T_event&& consume();

private:
  void once(const std::stop_token,
            const Time_point initial_tick = Clock::now() + min_duration_ +
                                            busy_wait_,
            const size_t initial_i = 0);
  void repeat(const std::stop_token,
              const Time_point initial_tick = Clock::now() + min_duration_ +
                                              busy_wait_,
              const size_t initial_i = 0);

  void await_scheduler_idle();

  mutable std::mutex transport_mutex_;

  std::jthread scheduler_;
  Container events_;
  Output_queue output_;
  std::atomic<Time_point> t_next_;
  T_event buffer_;
};

} // namespace sequencer

} // namespace Micro_composer

#include "sequencer.tpp"

#endif
