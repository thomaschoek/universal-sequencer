#include "atomic_step_sequence.h"

namespace Micro_composer {

namespace sequence {

template <Sequencable Step_t> Step_t Atomic_step_sequence<Step_t>::next() {
  if (base_t::empty()) {
    throw std::out_of_range(
        "Attempted to get next event from an empty sequence");
  }
  base_t::lock();
  if (iterator_ >= base_t::cend() || iterator_ < base_t::cbegin()) {
    iterator_ = base_t::cbegin();
  }
  // Return a copy of the current step's value, then increment the step iterator
  return *iterator_++;
}

} // namespace sequence

} // namespace Micro_composer
