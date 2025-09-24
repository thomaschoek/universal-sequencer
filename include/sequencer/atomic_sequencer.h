#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "utils/atomic_deque.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

template <Sequencable Event_t> class Atomic_sequencer {
public:
  using Handler_t = typename std::function<void(Event_t&&)>;
  using clock = std::chrono::steady_clock;
  using time_point = typename clock::time_point;
  using Sequence_t = typename atomic_deque::Atomic_deque<Event_t>;
  using Sequence_itr_t = typename Sequence_t::iterator;

  explicit Atomic_sequencer(Handler_t handler) : event_handler(handler) {}
  Atomic_sequencer(Handler_t handler, atomic_deque::Atomic_deque<Event_t>& seq)
      : event_handler(handler), sequence(seq) {}

  void start(time_point start_time = clock::now());
  void stop();
  bool is_running() const;

  void push_back(Event_t&& step);
  void push_front(Event_t&& step);
  Sequence_itr_t insert(const Sequence_itr_t step_idx, Event_t&& step);
  Event_t at(const Sequence_itr_t step_idx) const;
  void update(const Sequence_itr_t step_idx, Event_t&& step_params);
  void erase(const Sequence_itr_t step_idx);
  void erase(const Sequence_itr_t first, const Sequence_itr_t last);
  void pop_back();
  void pop_front();

private:
  void run(std::stop_token st, time_point start_time);
  Event_t next_event();
  Handler_t event_handler;

  Sequence_t& sequence;
  Sequence_itr_t event_itr;

  std::mutex mutex_;
  std::jthread thread_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
