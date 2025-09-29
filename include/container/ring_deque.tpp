#include "ring_deque.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

template <typename Step_t> Step_t Ring_deque<Step_t>::next() {
  if (Base_deque::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  Base_deque::lock();
  if (iterator_ >= Base_deque::cend() || iterator_ < Base_deque::cbegin()) {
    iterator_ = Base_deque::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

} // namespace container

} // namespace Micro_composer
