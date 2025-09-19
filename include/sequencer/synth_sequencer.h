#ifndef MICRO_COMPOSER_SYNTH_SEQUENCER_H
#define MICRO_COMPOSER_SYNTH_SEQUENCER_H

#include "sequencer/step_sequencer.h"
#include "sequencer/step_sequencer_output.h"
#include "synth/synth.h"

namespace MicroComposer {

namespace sequencer {

class SynthSequencer : public Sequencer {
private:
  mutable StepSequencerSynthOutput synth_output_;

public:
  SynthSequencer(const AtomicStepSequence &seq, double sample_rate = 44100.0,
                 MicroComposer::synth::WaveformType waveform =
                     MicroComposer::synth::WaveformType::SINE);

  // Configure the synthesizer
  void setWaveform(MicroComposer::synth::WaveformType waveform);
  void setAudioCallback(
      std::function<void(const std::vector<double> &, double)> callback);

  // Access to the synth output
  StepSequencerSynthOutput &synthOutput() { return synth_output_; }
  const StepSequencerSynthOutput &synthOutput() const { return synth_output_; }

protected:
  // Override trigger to send steps to synthesizer
  void trigger(const Step &step) const override;
};

} // namespace sequencer

} // namespace MicroComposer

#endif // SYNTH_SEQUENCER_H