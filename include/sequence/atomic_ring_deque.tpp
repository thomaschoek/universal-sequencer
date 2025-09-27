#include "atomic_ring_deque.h"

namespace Micro_composer {

namespace sequence {

template <Sequencable Step_t> Step_t Atomic_ring_deque<Step_t>::next() {
  if (Base_t::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  Base_t::lock();
  if (iterator_ >= Base_t::cend() || iterator_ < Base_t::cbegin()) {
    iterator_ = Base_t::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

} // namespace sequence

} // namespace Micro_composer
