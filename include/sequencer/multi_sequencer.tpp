#include "sequencer/multi_sequencer.h"

namespace Micro_composer {

namespace sequencer {

template <Sequencable Event_t>
Multi_sequencer<Event_t>::Multi_sequencer(Handler_t handler,
                                          Sequence_deque sequences) {
  for (auto& seq : sequences) {
    parallel_sequencers.emplace_back(handler, seq);
  }
}

template <Sequencable Event_t>
Multi_sequencer<Event_t>::Multi_sequencer(Handler_t handler,
                                          unsigned int n_parallel)
    : parallel_sequencers{n_parallel, Sequencer_t{handler}} {}

template <Sequencable Event_t>
void Multi_sequencer<Event_t>::start(Sequencer_time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(parallel_sequencers.size());

  for (auto& sequencer : parallel_sequencers) {
    futures.emplace_back(std::async(std::launch::async, &Sequencer_t::start,
                                    &sequencer, common_start_time));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

template <Sequencable Event_t> void Multi_sequencer<Event_t>::stop() {
  std::vector<std::future<void>> futures;
  futures.reserve(parallel_sequencers.size());

  for (auto& sequencer : parallel_sequencers) {
    futures.emplace_back(
        std::async(std::launch::async, &Sequencer_t::stop, &sequencer));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

} // namespace sequencer

} // namespace Micro_composer
