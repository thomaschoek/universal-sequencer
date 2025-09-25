#include "sequencer/atomic_sequencer.tpp"
#include "sequencer/multi_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

// CRUD
template <Sequencable Event_t>
void Multi_sequencer::add_sequence(
    std::function<void(Event_t&&)> handler,
    sequence::Atomic_step_sequence<Event_t>&& seq) {
  using Sequencer_t = Atomic_sequencer<Event_t>;
  auto sequencer = std::make_unique<Sequencer_t>(handler, std::move(seq));
  sequencers_.emplace_back(std::move(sequencer));
}

void Multi_sequencer::add_sequence(std::unique_ptr<Sequencer_base>&& seqr) {
  sequencers_.emplace_back(std::move(seqr));
}

// Control
void Multi_sequencer::start(Sequencer_time_point common_start_time) {
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

template <Sequencable Event_t> void Multi_sequencer::stop() {
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
