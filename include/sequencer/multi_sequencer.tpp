#include "sequencer/multi_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

template <Sequencable Event_t>
Multi_sequencer<Event_t>::Multi_sequencer(Sequencer_vec s_vec)
    : parallel_sequencers{s_vec} {}

template <Sequencable Event_t>
Multi_sequencer<Event_t>::Multi_sequencer(std::vector<Handler_t> handlers,
                                          std::vector<Sequence_t> sequences) {
  if (handlers.size() != sequences.size()) {
    throw std::invalid_argument(
        "Number of handlers must match number of sequences");
  }
  for (std::size_t i = 0; i < handlers.size(); ++i) {
    parallel_sequencers.emplace_back(
        std::make_unique<Sequencer_t>(handlers[i], sequences[i]));
  }
}

template <Sequencable Event_t>
void Multi_sequencer<Event_t>::start(Sequencer_time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(parallel_sequencers.size());

  for (auto& sequencer : parallel_sequencers) {
    futures.emplace_back(std::async(std::launch::async, &Sequencer_t::start,
                                    sequencer.get(), common_start_time));
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
        std::async(std::launch::async, &Sequencer_t::stop, sequencer.get()));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

} // namespace sequencer

} // namespace Micro_composer
