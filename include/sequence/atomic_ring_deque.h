#ifndef MICRO_COMPOSER_ATOMIC_STEP_SEQUENCE_H
#define MICRO_COMPOSER_ATOMIC_STEP_SEQUENCE_H

#include "atomic_deque.h"
#include "sequencable/concept.h"

namespace Micro_composer {

namespace sequence {

using atomic_deque::Atomic_deque;
using sequencable::Sequencable;

template <Sequencable Step_t>
class Atomic_ring_deque : public Atomic_deque<Step_t> {
public:
  using Base_t = Atomic_deque<Step_t>;
  using Itr_t = typename Atomic_deque<Step_t>::iterator;
  Step_t next();

protected:
  const Itr_t iterator_;
};

} // namespace sequence

} // namespace Micro_composer

#endif
