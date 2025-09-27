#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "sequence/atomic_ring_deque.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;
using sequence::Atomic_ring_deque;

template <Sequencable Event_t>
class Atomic_sequencer : public Atomic_ring_deque<Event_t> {
public:
  using Base_t = Atomic_ring_deque<Event_t>;
  using Handler_t = typename std::function<void(Event_t&&)>;
  using Clock = std::chrono::steady_clock;
  using Time_point = typename Clock::time_point;
  using Sequence_t = typename atomic_deque::Atomic_deque<Event_t>;
  using Size_t = typename Sequence_t::size_type;
  using Itr_t = typename Sequence_t::iterator;

  explicit Atomic_sequencer(Handler_t handler) : handler_(handler) {}
  Atomic_sequencer(Handler_t handler, atomic_deque::Atomic_deque<Event_t>& seq)
      : handler_(handler), Base_t(seq) {}

  bool is_running() const;
  void start(Time_point start_time = Clock::now());
  void stop();

private:
  void run(std::stop_token st, Time_point start_time);
  std::jthread runner_thread_;
  Handler_t handler_;

  std::mutex transport_mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
