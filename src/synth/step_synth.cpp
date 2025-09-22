#include "synth/step_synth.h"
#ifndef NDEBUG
#include <iostream>
#endif

namespace MicroComposer {

namespace synth {

using sequencer::Step;

Synthesizable StepSynth::parseStep(const Step &step) {
  Synthesizable synth_step;
  if (step.parameters.size() >= 2) {
    synth_step.frequency = step.parameters[0];
    synth_step.amplitude = step.parameters[1];
  } else if (step.parameters.size() == 1) {
    // Single parameter interpreted as frequency, use default amplitude
    synth_step.frequency = step.parameters[0];
    synth_step.amplitude = 0.5; // Medium volume
  } else {
    // Default values if parameters are missing
    synth_step.frequency = 440.0; // A4
    synth_step.amplitude = 0.5;   // Medium volume
  }

  synth_step.phase = (step.parameters.size() >= 3) ? step.parameters[2] : 0.0;
  synth_step.duration = step.length;

  return std::move(synth_step);
}

void StepSynth::write(const Step &step) {
  Synthesizable synth_step = parseStep(step);
#ifndef NDEBUG
  // Default behavior: output basic info to console
  std::cout << "[SYNTH] Playing: " << std::fixed << std::setprecision(2)
            << synth_step.frequency << "Hz, " << synth_step.amplitude
            << " amplitude, " << synth_step.duration.count() << std::endl;
#endif
  auto samples = generateSamples(std::move(synth_step));
  output_.write(samples);
}

} // namespace synth

} // namespace MicroComposer
