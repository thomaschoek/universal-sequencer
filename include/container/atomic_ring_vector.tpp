#include "container/atomic_ring_vector.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

// Constructors
template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(
    typename Base_vector::Initializer_list seq)
    : Base_vector(seq) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(const std::vector<T>& vec)
    : Base_vector(vec) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(std::vector<T>&& vec)
    : Base_vector(std::move(vec)) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
void Atomic_ring_vector<T>::set_pos(typename Base_vector::Index pos) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (Unatomic_base_vector::empty()) {
    iterator_ = Unatomic_base_vector::cbegin();
    return;
  }
  if (pos >= Unatomic_base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_vector::set_pos: Position out of range.");
  }
  iterator_ = Unatomic_base_vector::cbegin() + pos;
}

template <typename T> T Atomic_ring_vector<T>::next() {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (Unatomic_base_vector::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  if (iterator_ >= Unatomic_base_vector::cend() ||
      iterator_ < Unatomic_base_vector::cbegin()) {
    iterator_ = Unatomic_base_vector::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

template <typename T>
inline typename Atomic_ring_vector<T>::Base_vector::Index
Atomic_ring_vector<T>::get_pos() const {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (Unatomic_base_vector::empty()) {
    return 0;
  }
  return static_cast<const Base_vector::Index>(iterator_ -
                                               Unatomic_base_vector::cbegin());
}

} // namespace container
} // namespace Micro_composer
