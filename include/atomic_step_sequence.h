#ifndef ATOMIC_STEP_SEQUENCE_H
#define ATOMIC_STEP_SEQUENCE_H

#include "atomic_deque.h"
#include "step.h"
#include <optional>

namespace sequencer {

class AtomicStepSequence : public atomic_deque::AtomicDeque<Step> {
  typedef atomic_deque::AtomicDeque<Step> base_t;

private:
  mutable base_t::iterator step_itr_;

public:
  std::optional<Step> step();

  explicit AtomicStepSequence(base_t::size_type count) : base_t(count) {
    step_itr_ = begin();
  }
};
} // namespace sequencer

#endif // ATOMIC_STEP_SEQUENCE_H
