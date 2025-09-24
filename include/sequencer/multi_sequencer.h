#ifndef MICRO_COMPOSER_MULTI_SEQUENCER_H
#define MICRO_COMPOSER_MULTI_SEQUENCER_H

#include "sequencer/atomic_sequencer.tpp"

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

template <Sequencable Event_t> class Multi_sequencer {
  using Handler_t = Atomic_sequencer<Event_t>::Handler_t;
  using Sequence_t = atomic_deque::Atomic_deque<Event_t>;
  using Sequence_deque = atomic_deque::Atomic_deque<Sequence_t>;
  using Sequencer_t = Atomic_sequencer<Event_t>;
  using Sequencer_clock = Atomic_sequencer<Event_t>::clock;
  using Sequencer_time_point = Atomic_sequencer<Event_t>::time_point;
  using Sequencer_deque = atomic_deque::Atomic_deque<Sequencer_t>;

public:
  explicit Multi_sequencer(std::vector<Handler_t>, std::vector<Sequence_t>);
  void start(Sequencer_time_point = Sequencer_clock::now());
  void stop();

private:
  Sequencer_deque parallel_sequencers;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
