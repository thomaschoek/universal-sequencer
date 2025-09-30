#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "abstract.h"
#include "container/atomic_ring_deque.h"
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
                         public container::Atomic_ring_deque<Event_t> {
public:
  using Atomic_ring_deque = container::Atomic_ring_deque<Event_t>;
  using Protected_base_deque = Atomic_ring_deque::Base_deque;
  using Handler = std::function<void(Event_t&&)>;
  using Clock = Sequencer::Clock;
  using Time_point = Sequencer::Time_point;
  using Duration = Event_t::Duration;
  using Step_idx = Protected_base_deque::size_type;
  using Step_iterator = Protected_base_deque::iterator;
  using Initializer_list = std::initializer_list<Event_t>;

  // Constructors
  Atomic_sequencer() = default;
  Atomic_sequencer(const Atomic_sequencer&) noexcept;
  Atomic_sequencer& operator=(const Atomic_sequencer&) noexcept;
  Atomic_sequencer(Atomic_sequencer&&) noexcept;

  explicit Atomic_sequencer(Handler) noexcept;
  Atomic_sequencer(Initializer_list, Handler) noexcept;
  explicit Atomic_sequencer(const std::vector<Event_t>&) noexcept;
  explicit Atomic_sequencer(std::vector<Event_t>&&) noexcept;

  ~Atomic_sequencer();

  // Thread-safe transport control
  bool is_running() const override;
  void start(Time_point start_time = Clock::now()) override;
  void stop() override;

  void set_handler(const Handler);

  void set_duration(const Duration);
  void set_duration(const Step_idx, const Duration);

  void set_offset(const Duration);
  void set_offset(const Step_idx, const Duration);

private:
  void run(std::stop_token st, Time_point start_time);
  std::jthread runner_thread_;
  Handler handler_ = [](Event_t&&) {};

  // Track next scheduled event time for preserving state across moves
  std::atomic<Time_point> next_step_time_{Clock::now()};

  std::mutex mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/atomic_sequencer.tpp"

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
