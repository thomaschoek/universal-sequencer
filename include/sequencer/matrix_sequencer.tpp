#include "sequencer/matrix_sequencer.h"
#include <stdexcept>

namespace Micro_composer {

namespace sequencer {

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::update(Seq_idx seq_idx,
                                              Step_idx step_idx,
                                              Param_idx param_idx,
                                              T_event_params&& value) {
  std::scoped_lock lck{Base::transport_mutex_};

  if (seq_idx >= Base::size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer::update: Sequencer index out of range.");
  }

  auto& seq = Base::operator[](seq_idx);
  if (step_idx >= seq.size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer::update: Step index out of range.");
  }

  // Use the sequencer's update method which properly modifies the underlying element
  seq.update(step_idx, param_idx, std::forward<T_event_params>(value));
}

} // namespace sequencer

} // namespace Micro_composer
