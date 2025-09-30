#include "container/atomic_ring_deque.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

template <typename T>
Atomic_ring_deque<T>::Atomic_ring_deque(const std::vector<T>& vec) noexcept
    : Base_deque(vec) {
  // mutex_ is default-initialized
  iterator_ = Base_deque::cbegin();
}

template <typename T>
Atomic_ring_deque<T>::Atomic_ring_deque(std::vector<T>&& vec) noexcept
    : Base_deque(std::move(vec)) {
  // mutex_ is default-initialized
  iterator_ = Base_deque::cbegin();
}

template <typename T> T Atomic_ring_deque<T>::next() {
  std::scoped_lock lck = Atomic_deque<T>::get_lock();
  if (Base_deque::Base_deque::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  if (iterator_ >= Base_deque::cend() || iterator_ < Base_deque::cbegin()) {
    iterator_ = Base_deque::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

} // namespace container
} // namespace Micro_composer
