#ifndef MICRO_COMPOSER_MULTI_SEQUENCER_H
#define MICRO_COMPOSER_MULTI_SEQUENCER_H

#include "sequencable/concept.h"

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

class Multi_sequencer {
  // TODO should be a class that can hold a deque of sequencers of different
  // types so for instance element 0 is Atomic_sequencer<Event_t_1, Handler_t_1>
  // and element 1 is Atomic_sequencer<Event_t_2, Handler_t_2>, ...

public:
  void start();
  void stop();

private:
  // variable type container of sequencers
  std::vector<std::shared_ptr<void>> sequencers;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
