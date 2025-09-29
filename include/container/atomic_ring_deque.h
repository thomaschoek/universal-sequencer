#ifndef MICRO_COMPOSER_ATOMIC_RING_DEQUE_H
#define MICRO_COMPOSER_ATOMIC_RING_DEQUE_H

#include "container/atomic_deque.h"

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_ring_deque : public Atomic_deque<T> {
public:
  using Base_deque = Atomic_deque<T>;
  T next();

private:
  Base_deque::const_iterator iterator_{Atomic_deque<T>::begin()};
};

} // namespace container

} // namespace Micro_composer

#endif
