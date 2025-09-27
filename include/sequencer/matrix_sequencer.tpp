#include "matrix_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

void Matrix_sequencer::start(Seq_idx idx, Time_point start_time) {
  if (idx >= sequencers_.size()) {
    throw std::out_of_range("Index out of range in Matrix_sequencer::start");
  }
  sequencers_[idx].start(start_time);
}

void Matrix_sequencer::start_all(Time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
    futures.emplace_back(
        std::async(std::launch::async, [&sequencer]() { sequencer.start(); }));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

void Matrix_sequencer::stop(Seq_idx idx) {
  if (idx >= sequencers_.size()) {
    throw std::out_of_range("Index out of range in Matrix_sequencer::stop");
  }
  sequencers_[idx].stop();
}

void Matrix_sequencer::stop_all() {
  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
    futures.emplace_back(std::async([&sequencer]() { sequencer.stop(); }));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

} // namespace sequencer

} // namespace Micro_composer
