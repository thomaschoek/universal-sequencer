#include "sequencer/step_sequencer_output.h"
#ifndef NDEBUG
#include <iostream>
#endif
#include <chrono>

namespace MicroComposer {

namespace sequencer {

void StepSequencerOutput::write(const Step &step) {
#ifndef NDEBUG
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            << ": " << step << std::endl;
#endif
  // Default implementation does nothing
  (void)step; // Suppress unused parameter warning
}

} // namespace sequencer

} // namespace MicroComposer
