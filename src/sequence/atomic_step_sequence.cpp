#include "sequence/atomic_step_sequence.h"
#include <optional>

using namespace MicroComposer;
using namespace MicroComposer::sequencer;

std::optional<Step> AtomicStepSequence::step() {
  std::scoped_lock lck{mutex_};
  if (begin() == end()) {
    return std::nullopt;
  }
  if (step_itr_ >= end() || step_itr_ < begin()) {
    step_itr_ = begin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *step_itr_++;
}
