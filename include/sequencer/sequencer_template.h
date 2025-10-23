#ifndef MICRO_COMPOSER_SEQUENCER_TEMPLATE_H
#define MICRO_COMPOSER_SEQUENCER_TEMPLATE_H

#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "concurrency/thread_pool.h"
#include "sequencable/concepts.h"

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable T_event> struct Sequencer {
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Container = std::vector<std::unique_ptr<T_event>>;
  using Size_type = Container::size_type;
  using Thread_pool = thread_pool::Thread_pool<T_event>;
  using Events_initializer = std::initializer_list<T_event>;
  using Handler = std::function<void(T_event&&)>;

  explicit Sequencer(Handler, Events_initializer = {});
  Sequencer(Handler, const Container&);
  Sequencer(Handler, Container&&);
  Sequencer(Handler, Sequencer&&) noexcept;

  // Set handler post-construction
  void set_handler(const Handler&);

  // Thread-safe transport control
  void start(const Time_point = Clock::now() + min_duration_,
             const bool repeat = false);
  void pause(const Time_point = Clock::now() + min_duration_);
  void stop(const Time_point = Clock::now(), const Size_type stop_pos = 0);
  bool is_scheduling() const;

  // Get the time of the next scheduled event
  Time_point t_next() const;

  // Thread-safe time signature CRUD operations
  std::vector<Duration> time_signature() const noexcept;
  std::vector<T_event> data() const noexcept;
  bool empty();
  Size_type size();
  void set_pos(Size_type = 0);
  Size_type get_pos() const noexcept;
  void assign(Size_type, const T_event&);
  void push_back(const T_event&);
  void pop_back();
  void insert(Size_type, const T_event&);
  void erase(Size_type);
  void assign(Events_initializer);
  void assign(const std::vector<T_event>&);
  void clear() noexcept;

protected:
  static constexpr const Duration min_duration_{std::chrono::milliseconds{10}};
  static constexpr const Duration operation_timeout_{std::chrono::seconds{30}};
  static constexpr const Duration spin_duration_{std::chrono::milliseconds{5}};

  // Wait until outside of window where scheduler is loading events_[current_]
  Time_point await_scheduler() const noexcept;

  std::scoped_lock<std::mutex> lock_events(Time_point&) const;

private:
  Time_point once(const std::stop_token,
                  const Time_point initial_time = Clock::now() + min_duration_,
                  const Size_type initial_index = 0);
  void repeat(const std::stop_token,
              const Time_point initial_time = Clock::now() + min_duration_,
              const Size_type initial_index = 0);

  mutable std::mutex transport_mutex_;
  mutable std::mutex data_mutex_;

  Container events_;
  std::atomic<Size_type> current_{0};

  Thread_pool pool_;

  std::jthread scheduler_;
  std::atomic<Time_point> t_next_{Time_point::min()};
};

} // namespace sequencer

} // namespace Micro_composer

#include "sequencer_template.tpp"

#endif
