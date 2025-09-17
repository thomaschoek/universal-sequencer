#include "atomic_step_sequence.h"
#include <optional>

namespace sequencer {

std::optional<Step> AtomicStepSequence::step() {
  std::scoped_lock lck{mutex_};
  if (begin() == end()) {
    return std::nullopt;
  }
  if (step_itr_ >= end() || step_itr_ < begin()) {
    step_itr_ = begin();
  }
  return *step_itr_++;
}

} // namespace sequencer
