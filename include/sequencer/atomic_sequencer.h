#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "abstract.h"
#include "container/ring_deque.h"
#include "sequencable/concepts.h"
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable_updatable;

template <Sequencable_updatable Event_t>
class Atomic_sequencer : public abstract::Sequencer,
                         private container::Ring_deque<Event_t> {
public:
  using Base_steps = container::Ring_deque<Event_t>;
  using Handler = std::function<void(Event_t&&)>;
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Step_idx = Base_steps::size_type;
  using Step_iterator = Base_steps::iterator;
  using Initializer_list = std::initializer_list<Event_t>;

  // Constructors
  Atomic_sequencer() = default;
  Atomic_sequencer(const Atomic_sequencer&);
  Atomic_sequencer& operator=(const Atomic_sequencer&);
  Atomic_sequencer(Atomic_sequencer&&) noexcept;

  explicit Atomic_sequencer(Handler);
  Atomic_sequencer(Initializer_list, Handler);

  // Thread-safe transport control
  bool is_running() const override;
  void start(Time_point start_time = Clock::now()) override;
  void stop() override;

  // Thread-safe CRUD operations
  void set_handler(const Handler);
  void assign(Initializer_list);

  void push_back(const Event_t&);
  void push_back(Event_t&&);
  void push_front(const Event_t&);
  void push_front(Event_t&&);

  void insert(Step_idx, const Event_t&);
  void insert(Step_idx, Event_t&&);

  template <typename... Args> void update(Step_idx, Args...);
  void replace(Step_idx, const Event_t&);
  void replace(Step_idx, Event_t&&);

  void pop_back();
  void pop_front();

  void erase(Step_idx);

  void clear() noexcept;
  Step_idx size() const noexcept;
  bool empty() const noexcept;

  Event_t front();
  Event_t back();

  Event_t at(Step_idx);

  const Base_steps& steps() const;

private:
  void run(std::stop_token st, Time_point start_time);
  std::jthread runner_thread_;
  Handler handler_ = [](Event_t&&) {};

  std::mutex mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
