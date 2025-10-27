#ifndef MICRO_COMPOSER_SEQUENCER_TEMPLATE_H
#define MICRO_COMPOSER_SEQUENCER_TEMPLATE_H

#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "common_types.h"
#include "concurrency/thread_pool.h"
#include "container/atomic_vector.h"
#include "sequencable/concepts.h"

namespace Micro_composer {

namespace sequencer {

template <sequencable::Mut_seq_event T_event> struct Sequencer {
  using Clock = Common_types::Clock;
  using Time_point = Common_types::Time_point;
  using Duration = Common_types::Duration;
  using Container = container::Atomic_vector<T_event>;
  using Mutator = Container::Mutator;
  using Size_type = Container::Size_type;
  using Thread_pool = thread_pool::Thread_pool<T_event>;
  using Events_initializer = std::initializer_list<T_event>;
  using Handler = std::function<void(T_event&&)>;

  // Constructors
  explicit Sequencer(Handler, Events_initializer = {});
  Sequencer(Handler, const Container&);
  Sequencer(Handler, Container&&);
  Sequencer(Sequencer&&) noexcept;

  // Destructor
  ~Sequencer();

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
  // Read operations
  struct State {
    bool is_scheduling;
    Size_type next;
    Time_point t_next;
    Size_type size;
    Container events;
  };
  const State get_state() const;
  bool empty() const noexcept;
  Size_type size() const noexcept;
  Size_type get_pos() const noexcept;
  std::vector<T_event> snapshot() const noexcept;

  // Setters / Modifiers
  void set_pos(Size_type = 0);
  void update(Size_type pos, const T_event&);
  template <typename... Args> void update(Size_type pos, Args&&... update_args);

  void push_back(const T_event&);
  void insert(Size_type, const T_event&);

  void adjust_durations(Duration delta);
  void multiply_durations(double factor);
  void enable();
  void enable(Size_type);
  void disable();
  void disable(Size_type);
  void toggle();
  void toggle(Size_type);

  void mutate(const Mutator&);
  void mutate(Size_type, const Mutator&);

  void replace(Size_type, const T_event&);
  void replace(Size_type start, const std::vector<T_event>&);
  void assign(Events_initializer);
  void assign(const Container&);
  void assign(Size_type, const T_event&);

  // Delete operations
  void pop_back();
  void erase(Size_type);
  void clear();

protected:
  static void validate(const std::vector<T_event>&);
  static void validate(const T_event&);
  static void validate(const Time_point&);
  void range_check(Size_type) const;
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

  std::jthread scheduler_;
  Thread_pool pool_;
  mutable std::mutex transport_mutex_;
  mutable std::mutex data_mutex_;
  Container events_;
  std::atomic<Size_type> next_{0};
  std::atomic<Time_point> t_next_{Time_point::min()};
};

} // namespace sequencer

} // namespace Micro_composer

#include "sequencer_template.tpp"

#endif
