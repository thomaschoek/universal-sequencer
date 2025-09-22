#ifndef MICRO_COMPOSER_SYNTH_STEP_H
#define MICRO_COMPOSER_SYNTH_STEP_H

#include "sequencable/step.h"
#include "synth/synth.h"

namespace MicroComposer {

namespace sequencable {

struct SynthStep : public Step, synth::OscillationParams {

  SynthStep() = default;
  SynthStep(double offset_seconds, double length_seconds,
            const synth::OscillationParams &params = {})
      : Step(offset_seconds, length_seconds,
             {params.frequency, params.amplitude, params.phase}),
        synth::OscillationParams(params) {};
  SynthStep(double offset_seconds, double length_seconds,
            std::vector<double> &&params)
      : Step(offset_seconds, length_seconds, std::move(params)) {};
};

} // namespace sequencable

} // namespace MicroComposer

#endif
