#ifndef MICRO_COMPOSER_MULTI_SEQUENCER_H
#define MICRO_COMPOSER_MULTI_SEQUENCER_H

#include "sequencer/atomic_sequencer.h"
#include <memory>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

template <Sequencable Event_t> class Multi_sequencer {
public:
  using Handler_t = Atomic_sequencer<Event_t>::Handler_t;
  using Sequence_t = sequence::Atomic_step_sequence<Event_t>;
  using Sequence_deque = atomic_deque::Atomic_deque<Sequence_t>;
  using Sequencer_t = Atomic_sequencer<Event_t>;
  using Sequencer_clock = Atomic_sequencer<Event_t>::clock;
  using Sequencer_time_point = Atomic_sequencer<Event_t>::time_point;
  using Sequencer_vec = std::vector<std::unique_ptr<Sequencer_t>>;

  explicit Multi_sequencer(Sequencer_vec);
  Multi_sequencer(std::vector<Handler_t>, std::vector<Sequence_t>);
  void start(Sequencer_time_point = Sequencer_clock::now());
  void stop();

private:
  Sequencer_vec parallel_sequencers;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
