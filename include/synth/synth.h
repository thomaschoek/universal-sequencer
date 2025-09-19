#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include <chrono>
#include <functional>
#include <vector>

namespace MicroComposer {
namespace synth {

struct Synthesizable {
  double frequency;
  double amplitude;
  double phase;
  std::chrono::duration<double> duration;
};

enum class WaveformType { SINE, SQUARE, SAWTOOTH, TRIANGLE };

class Synthesizer {
private:
  double sample_rate_;
  std::function<void(const std::vector<double> &, double)> audio_callback_;

  // Waveform generators
  double generateSine(double frequency, double phase, double time) const;
  double generateSquare(double frequency, double phase, double time) const;
  double generateSawtooth(double frequency, double phase, double time) const;
  double generateTriangle(double frequency, double phase, double time) const;

public:
  Synthesizer(double sample_rate = 44100.0);

  void synthesize(const Synthesizable &params,
                  WaveformType waveform = WaveformType::SINE);

  // Set callback for audio output (e.g., to audio system, file, or console)
  void setAudioCallback(
      std::function<void(const std::vector<double> &, double)> callback);

  // Generate audio samples for the given parameters
  std::vector<double>
  generateSamples(const Synthesizable &params,
                  WaveformType waveform = WaveformType::SINE) const;
};

} // namespace synth
} // namespace MicroComposer

#endif
