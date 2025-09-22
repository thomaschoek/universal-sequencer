#ifndef MICRO_COMPOSER_STEP_SYNTH_H
#define MICRO_COMPOSER_STEP_SYNTH_H

#include "sequencer/step_sequencer_output.h"
#include "synth/synth.h"

namespace MicroComposer {

namespace synth {

class StepSynth : public Synthesizer, public sequencer::StepSequencerOutput {

protected:
  static Synthesizable parseStep(const sequencer::Step &step);

public:
  void write(const sequencer::Step &step);
};

} // namespace synth

} // namespace MicroComposer

#endif
