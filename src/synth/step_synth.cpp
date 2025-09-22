#include "synth/step_synth.h"
#ifndef NDEBUG
#include <iostream>
#endif

namespace MicroComposer {

namespace synth {

OscillationParams
StepSynth::parseStepParams(const std::vector<double> &step_params) {
  OscillationParams osc_params;
  for (std::size_t i = 0; i < step_params.size(); ++i) {
    switch (static_cast<ParameterIndex>(i)) {
    case ParameterIndex::FREQUENCY:
      osc_params.frequency = step_params[i];
      break;
    case ParameterIndex::DURATION:
      osc_params.duration = std::chrono::duration<double>(step_params[i]);
      break;
    case ParameterIndex::AMPLITUDE:
      osc_params.amplitude = step_params[i];
      break;
    case ParameterIndex::PHASE:
      osc_params.phase = step_params[i];
      break;
    default:
      // Ignore unknown parameters
      break;
    }
  }
  return osc_params;
}

void StepSynth::write(const std::vector<double> &&step_params) {
  OscillationParams synth_step = parseStepParams(step_params);
#ifndef NDEBUG
  // Default behavior: output basic info to console
  std::cout << "[SYNTH] Playing: " << std::fixed << std::setprecision(2)
            << synth_step.frequency << "Hz, " << synth_step.amplitude
            << " amplitude, " << synth_step.duration.count() << std::endl;
#endif
  auto samples = generateSamples(synth_step);
  output_.write(samples);
}

} // namespace synth

} // namespace MicroComposer
