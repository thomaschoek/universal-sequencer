#ifndef MICRO_COMPOSER_PARALLEL_SEQUENCER_H
#define MICRO_COMPOSER_PARALLEL_SEQUENCER_H

#include "sequencable/concept.h"
#include "sequencer/atomic_sequencer.h"
#include "utils/atomic_deque.h"

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

template <Sequencable Event_t, typename Handler_t> class Parallel_sequencer {
  typedef atomic_deque::Atomic_deque<Event_t> Event_deque;
  typedef atomic_deque::Atomic_deque<Event_deque> Sequence_deque;
  typedef atomic_deque::Atomic_deque<Atomic_sequencer<Event_t, Handler_t>>
      Sequencer_deque;
  typedef Atomic_sequencer<Event_t, Handler_t> sub_sequencer_t;

public:
  explicit Parallel_sequencer(Handler_t);
  explicit Parallel_sequencer(Sequencer_deque) : sequencers(sequencers) {};
  Parallel_sequencer(Handler_t, Sequence_deque);

  void start();
  void stop();

private:
  Sequencer_deque sequencers;
  Sequence_deque events;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
