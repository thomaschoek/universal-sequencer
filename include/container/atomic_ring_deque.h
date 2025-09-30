#ifndef MICRO_COMPOSER_ATOMIC_RING_DEQUE_H
#define MICRO_COMPOSER_ATOMIC_RING_DEQUE_H

#include "container/atomic_deque.h"

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_ring_deque : public Atomic_deque<T> {
public:
  using Base_deque = Atomic_deque<T>;

  // Constructors
  Atomic_ring_deque() = default;
  Atomic_ring_deque(const Atomic_ring_deque&) = default;
  Atomic_ring_deque& operator=(const Atomic_ring_deque&) = default;
  Atomic_ring_deque(Atomic_ring_deque&&) noexcept = default;
  Atomic_ring_deque& operator=(Atomic_ring_deque&&) noexcept = default;

  Atomic_ring_deque(typename Base_deque::Initializer_list seq)
      : Base_deque(seq) {}

  T next();

private:
  Base_deque::const_iterator iterator_{Base_deque::begin()};
};

} // namespace container

} // namespace Micro_composer

#endif
