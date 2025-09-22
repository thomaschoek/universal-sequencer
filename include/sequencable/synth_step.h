#ifndef MICRO_COMPOSER_SYNTH_STEP_H
#define MICRO_COMPOSER_SYNTH_STEP_H

#include "sequencable/step.h"
#include "synth/synth.h"

namespace MicroComposer {

namespace sequencable {

struct SynthStep : public Step {
  enum class ParameterIndex {
    FREQUENCY = 0,
    DURATION = 1,
    AMPLITUDE = 2,
    PHASE = 3
  };

  using Step::Step; // Inherit constructors

  // Conversion operator to allow implicit conversion to OscillationParams
  operator synth::OscillationParams() const {
    synth::OscillationParams params;
    for (std::size_t i = 0; i < parameters.size(); ++i) {
      switch (static_cast<ParameterIndex>(i)) {
      case ParameterIndex::FREQUENCY:
        params.frequency = parameters[i];
        break;
      case ParameterIndex::DURATION:
        params.duration = std::chrono::duration<double>(parameters[i]);
        break;
      case ParameterIndex::AMPLITUDE:
        params.amplitude = parameters[i];
        break;
      case ParameterIndex::PHASE:
        params.phase = parameters[i];
        break;
      default:
        // Ignore unknown parameters
        break;
      }
    }
    return params;
  }
};

} // namespace sequencable

} // namespace MicroComposer

#endif
