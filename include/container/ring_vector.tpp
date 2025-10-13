#include "ring_vector.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

// Constructors
template <typename T>
Ring_vector<T>::Ring_vector(Initializer_list seq)
    : Base_vector(seq), iterator_(Base_vector::cbegin()) {}

template <typename T>
Ring_vector<T>::Ring_vector(const std::vector<T>& vec)
    : Base_vector(vec), iterator_(Base_vector::cbegin()) {}

template <typename T>
Ring_vector<T>::Ring_vector(std::vector<T>&& vec)
    : Base_vector(std::move(vec)), iterator_(Base_vector::cbegin()) {}

template <typename T> void Ring_vector<T>::set_next(Size_type pos) {
  if (Base_vector::empty()) {
    iterator_ = Base_vector::cbegin();
    return;
  }
  if (pos >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Ring_vector::set_next: Position out of range.");
  }
  iterator_ = Base_vector::cbegin() + pos;
}

template <typename T> Ring_vector<T>::Const_iterator Ring_vector<T>::next() {
  if (iterator_ >= Base_vector::cend()) {
    iterator_ = Base_vector::cbegin();
  }

  return iterator_++;
}

} // namespace container
} // namespace Micro_composer
