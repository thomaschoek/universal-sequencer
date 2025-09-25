#include "sequencer/atomic_sequencer.tpp"
#include "sequencer/multi_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

// CRUD
template <Sequencable Event_t>
void Multi_sequencer<Event_t>::add_sequence(Handler_t handler,
                                            Sequence_t&& seq) {
  sequencers_.emplace_back(
      std::make_unique<Sequencer_t>(handler, std::move(seq)));
}

// Control
template <Sequencable Event_t>
void Multi_sequencer<Event_t>::start(Sequencer_time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
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
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
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
