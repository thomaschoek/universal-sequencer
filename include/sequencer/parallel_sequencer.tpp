#include "sequencer/parallel_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

template <Sequencable Event_t, typename Handler_t>
Parallel_sequencer<Event_t, Handler_t>::Parallel_sequencer(Handler_t handler) {
  sequencers.push_back(Atomic_sequencer{handler});
}

template <Sequencable Event_t, typename Handler_t>
Parallel_sequencer<Event_t, Handler_t>::Parallel_sequencer(
    Handler_t handler_, Sequence_deque events_)
    : events(events_) {
  for (Event_deque sequence : events) {
    sequencers.push_back(
        Atomic_sequencer<Event_t, Handler_t>{handler_, sequence});
  }
}

template <Sequencable Event_t, typename Handler_t>
void Parallel_sequencer<Event_t, Handler_t>::start() {
  auto start_time = clock::now();
  for (auto sequencer : sequencers) {
    sequencer.start(start_time);
  }
}

template <Sequencable Event_t, typename Handler_t>
void Parallel_sequencer<Event_t, Handler_t>::stop() {
  for (auto sequencer : sequencers) {
    std::ignore = std::async(sequencer.stop);
    std::ignore = std::async(std::launch::async, sequencer.stop);
  }
}

} // namespace sequencer

} // namespace Micro_composer
