#ifndef MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H
#define MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H

#include "sequence/step.h"
#include "synth/synth.h"
#include <functional>

namespace MicroComposer {

namespace sequencer {

class StepSequencerOutput {
public:
  virtual ~StepSequencerOutput() = default;
  virtual void write(const Step &step);
};

class StepSequencerSynthOutput : public StepSequencerOutput {
private:
  MicroComposer::synth::Synthesizer output_;
  MicroComposer::synth::WaveformType default_waveform_;

public:
  StepSequencerSynthOutput(double sample_rate = 44100.0,
                           MicroComposer::synth::WaveformType waveform =
                               MicroComposer::synth::WaveformType::SINE);

  void write(const Step &step);

  // Configure the synthesizer
  void setWaveform(MicroComposer::synth::WaveformType waveform);
  void setAudioCallback(
      std::function<void(const std::vector<double> &, double)> callback);

  // Access to underlying synthesizer
  MicroComposer::synth::Synthesizer &synthesizer() { return output_; }
  const MicroComposer::synth::Synthesizer &synthesizer() const {
    return output_;
  }
};

} // namespace sequencer

} // namespace MicroComposer

#endif // MICRO_COMPOSER_STEP_SEQUENCER_OUTPUT_H
