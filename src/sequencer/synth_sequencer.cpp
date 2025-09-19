#include "sequencer/synth_sequencer.h"

namespace MicroComposer {

namespace sequencer {

SynthSequencer::SynthSequencer(const AtomicStepSequence &seq,
                               double sample_rate,
                               MicroComposer::synth::WaveformType waveform)
    : Sequencer(seq), synth_output_(sample_rate, waveform) {}

void SynthSequencer::setWaveform(MicroComposer::synth::WaveformType waveform) {
  synth_output_.setWaveform(waveform);
}

void SynthSequencer::setAudioCallback(
    std::function<void(const std::vector<double> &, double)> callback) {
  synth_output_.setAudioCallback(callback);
}

void SynthSequencer::trigger(const Step &step) const {
  // Send step to synthesizer instead of just logging
  synth_output_.send(step);
}

} // namespace sequencer
} // namespace MicroComposer