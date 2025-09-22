#ifndef MICRO_COMPOSER_STEP_SYNTH_H
#define MICRO_COMPOSER_STEP_SYNTH_H

#include "sequencer/step_sequencer_output.h"
#include "synth/synth.h"

namespace MicroComposer {

namespace synth {

class StepSynth : public Synthesizer, public sequencer::StepSequencerOutput {

protected:
  enum class ParameterIndex {
    FREQUENCY = 0,
    DURATION = 1,
    AMPLITUDE = 2,
    PHASE = 3
  };
  static OscillationParams
  parseStepParams(const std::vector<double> &step_params);

public:
  explicit StepSynth(SynthOutput &output) : Synthesizer(output) {}
  void write(const std::vector<double> &&step_params) override;
};

} // namespace synth

} // namespace MicroComposer

#endif
