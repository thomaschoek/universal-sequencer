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
  using Base_deque = container::Atomic_ring_deque<Event_t>;
  using Handler = std::function<void(Event_t&&)>;
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Event_t::Duration;
  using Step_idx = Base_deque::size_type;
  using Step_iterator = Base_deque::iterator;
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

  void set_handler(const Handler);

private:
  void run(std::stop_token st, Time_point start_time);
  std::jthread runner_thread_;
  Handler handler_ = [](Event_t&&) {};

  std::mutex mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
