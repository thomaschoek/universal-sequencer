#ifndef MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H
#define MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H

#include "sequence/step.h"

namespace MicroComposer {

namespace sequencer {

class StepSequencerOutput {
public:
  virtual ~StepSequencerOutput() = default;
  virtual void write(const Step &step);
};

} // namespace sequencer

} // namespace MicroComposer

#endif // MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H
