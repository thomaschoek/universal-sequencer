#include "sequencer/step_sequencer_output.h"

namespace MicroComposer {

namespace sequencer {

StepSequencerSynthOutput::StepSequencerSynthOutput(
    double sample_rate, MicroComposer::synth::WaveformType waveform)
    : output_(sample_rate), default_waveform_(waveform) {}

void StepSequencerSynthOutput::send(const Step &step) {
  send(step, default_waveform_);
}

void StepSequencerSynthOutput::send(
    const Step &step, MicroComposer::synth::WaveformType waveform) {
  MicroComposer::synth::Synthesizable synth_step;

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

  output_.synthesize(synth_step, waveform);
}

void StepSequencerSynthOutput::setWaveform(
    MicroComposer::synth::WaveformType waveform) {
  default_waveform_ = waveform;
}

void StepSequencerSynthOutput::setAudioCallback(
    std::function<void(const std::vector<double> &, double)> callback) {
  output_.setAudioCallback(callback);
}

} // namespace sequencer

} // namespace MicroComposer
