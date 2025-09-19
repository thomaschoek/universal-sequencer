#ifndef MICRO_COMPOSER_STEP_SEQUENCER_H
#define MICRO_COMPOSER_STEP_SEQUENCER_H

#include "sequence/atomic_step_sequence.h"
#include "sequencer/step_sequencer_output.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace MicroComposer {

namespace sequencer {

class StepSequencer {

  std::mutex mutex_;
  std::jthread thread_;
  std::atomic<bool> live{false};
  AtomicStepSequence &sequence;
  StepSequencerOutput &output;
  void run();

public:
  bool is_live() const;
  void store_live(const bool &val);

  void start();
  void stop();

  StepSequencer(AtomicStepSequence &seq, StepSequencerOutput &out)
      : sequence(seq), output(out) {}
  StepSequencer(AtomicStepSequence &&seq, StepSequencerOutput &out)
      : sequence(seq), output(out) {}
  ~StepSequencer() { stop(); }
};

} // namespace sequencer
} // namespace MicroComposer

#endif
