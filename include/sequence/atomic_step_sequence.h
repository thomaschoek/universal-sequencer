#ifndef MICRO_COMPOSER_ATOMIC_STEP_SEQUENCE_H
#define MICRO_COMPOSER_ATOMIC_STEP_SEQUENCE_H

#include "sequencable/concept.h"
#include "utils/atomic_deque.h"

namespace Micro_composer {

namespace sequence {

using atomic_deque::Atomic_deque;
using sequencable::Sequencable;

template <Sequencable Step_t>
class Atomic_step_sequence : public Atomic_deque<Step_t> {
public:
  using base_t = Atomic_deque<Step_t>;
  using step_itr_t = typename Atomic_deque<Step_t>::iterator;

  Atomic_step_sequence() = default;
  Atomic_step_sequence(const Atomic_deque<Step_t>& deque) : base_t(deque) {}

  Step_t next();

private:
  step_itr_t iterator_;
};

} // namespace sequence

} // namespace Micro_composer

#endif
