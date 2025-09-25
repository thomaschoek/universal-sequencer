#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "sequencable/concept.h"
#include "sequence/atomic_step_sequence.h"
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
  using Sequence_t = typename sequence::Atomic_step_sequence<Event_t>;
  using size_type = typename Sequence_t::size_type;
  using Sequence_itr_t = typename Sequence_t::iterator;

  Atomic_sequencer();
  explicit Atomic_sequencer(Handler_t handler) : event_handler(handler) {}
  Atomic_sequencer(Handler_t handler, Sequence_t& seq)
      : event_handler(handler), sequence_(std::make_unique<Sequence_t>(seq)) {}

  void start(time_point start_time = clock::now());
  void stop();
  bool is_running() const;

  void push_back(Event_t&& step);
  void push_front(Event_t&& step);
  Sequence_itr_t insert(const size_type step_idx, Event_t&& step);
  Event_t at(const size_type step_idx) const;
  void update(const size_type step_idx, Event_t&& step_params);
  void erase(const size_type step_idx);
  void erase(const Sequence_itr_t first, const Sequence_itr_t last);
  void pop_back();
  void pop_front();

private:
  void run(std::stop_token st, time_point start_time);
  Handler_t event_handler;

  std::unique_ptr<Sequence_t> sequence_;

  std::mutex mutex_;
  std::jthread thread_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
