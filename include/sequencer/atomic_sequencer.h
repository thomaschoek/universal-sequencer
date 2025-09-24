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
  using Handler_t = std::function<void(Event_t&&)>;
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;

  explicit Atomic_sequencer(Handler_t handler) : event_handler(handler) {}
  Atomic_sequencer(Handler_t handler, atomic_deque::Atomic_deque<Event_t>& seq)
      : event_handler(handler), sequence(seq) {}

  void start(time_point start_time = clock::now());
  void stop();
  bool is_running() const;

  void push_back(Event_t&& step);
  void push_front(Event_t&& step);
  void insert(std::size_t step_idx, Event_t&& step);
  Event_t get(std::size_t step_idx) const;
  void update(std::size_t step_idx, Event_t&& step_params);
  void remove(std::size_t step_idx);
  Event_t pop_back();
  Event_t pop_front();

private:
  void run(std::stop_token st, time_point start_time);
  Event_t next_event();
  Handler_t event_handler;

  atomic_deque::Atomic_deque<Event_t>& sequence;
  atomic_deque::Atomic_deque<Event_t>::iterator event_itr;

  std::mutex mutex_;
  std::jthread thread_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
