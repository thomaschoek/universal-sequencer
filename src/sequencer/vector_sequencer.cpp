#include "sequencer/vector_sequencer.h"

namespace Micro_composer {

namespace sequencer {

void Vector_sequencer::update_step(Step_idx step_idx, Param_idx param_idx) {
  auto& step = Base::Base_steps::at(step_idx);
  if (param_idx >= step.parameters.size()) {
    throw std::out_of_range("Parameter index out of range");
  }
  // Example update: increment the parameter by 1.0
  step.parameters[param_idx] += 1.0;
}

} // namespace sequencer

} // namespace Micro_composer
