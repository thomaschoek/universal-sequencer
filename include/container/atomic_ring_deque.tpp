#include "container/atomic_ring_deque.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

// Constructors
template <typename T>
Atomic_ring_deque<T>::Atomic_ring_deque(
    typename Base_deque::Initializer_list seq)
    : Base_deque(seq) {
  // mutex_ is default-initialized
  iterator_ = Base_deque::cbegin();
}

template <typename T>
Atomic_ring_deque<T>::Atomic_ring_deque(const std::vector<T>& vec)
    : Base_deque(vec) {
  // mutex_ is default-initialized
  iterator_ = Base_deque::cbegin();
}

template <typename T>
Atomic_ring_deque<T>::Atomic_ring_deque(std::vector<T>&& vec)
    : Base_deque(std::move(vec)) {
  // mutex_ is default-initialized
  iterator_ = Base_deque::cbegin();
}

template <typename T>
void Atomic_ring_deque<T>::set_pos(typename Base_deque::Index pos) {
  std::scoped_lock lck = Atomic_deque<T>::get_lock();
  if (Unatomic_base_deque::empty()) {
    iterator_ = Unatomic_base_deque::cbegin();
    return;
  }
  if (pos >= Unatomic_base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::set_pos: Position out of range.");
  }
  iterator_ = Unatomic_base_deque::cbegin() + pos;
}

template <typename T> T Atomic_ring_deque<T>::next() {
  std::scoped_lock lck = Atomic_deque<T>::get_lock();
  if (Unatomic_base_deque::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  if (iterator_ >= Unatomic_base_deque::cend() ||
      iterator_ < Unatomic_base_deque::cbegin()) {
    iterator_ = Unatomic_base_deque::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

template <typename T>
inline typename Atomic_ring_deque<T>::Base_deque::Index
Atomic_ring_deque<T>::get_pos() const {
  std::scoped_lock lck = Atomic_deque<T>::get_lock();
  if (Unatomic_base_deque::empty()) {
    return 0;
  }
  return static_cast<const Base_deque::Index>(iterator_ -
                                              Unatomic_base_deque::cbegin());
}

} // namespace container
} // namespace Micro_composer
