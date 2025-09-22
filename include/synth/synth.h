#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include <chrono>
#include <cmath>
#include <vector>

namespace MicroComposer {
namespace synth {

enum class WaveformType { SINE, SQUARE, SAWTOOTH, TRIANGLE };

struct OscillationParams {
  double frequency;
  double amplitude;
  double phase;
  std::chrono::duration<double> duration;
  std::vector<double>::size_type
  compute_n_samples(std::vector<double>::size_type sample_rate) const {
    return duration.count() * sample_rate;
  }
};

struct Synthesizable : public OscillationParams {
  WaveformType waveform;
};

class SynthOutput {
public:
  void write(const std::vector<double> samples);
};

class Synthesizer {
private:
  static constexpr double TWO_PI = 2.0 * M_PI;

protected:
  std::vector<double>::size_type sample_rate_;

  SynthOutput &output_;

  // Waveform generators
  inline double sine_sample(const double frequency,
                            const std::vector<double>::size_type phase) const;
  std::vector<double> sine_oscillation(const OscillationParams params) const;

  inline double square_sample(const double frequency,
                              const std::vector<double>::size_type phase) const;
  std::vector<double> square_oscillation(const OscillationParams params) const;

  inline double saw_sample(const double frequency,
                           const std::vector<double>::size_type phase) const;
  std::vector<double> saw_oscillation(const OscillationParams params) const;

  inline double
  triangle_sample(const OscillationParams &params,
                  const std::vector<double>::size_type phase) const;
  std::vector<double>
  triangle_oscillation(const OscillationParams params) const;

public:
  Synthesizer(SynthOutput &output, const double sample_rate = 44100.0)
      : output_(output), sample_rate_(sample_rate) {};

  // Generate audio samples for the given parameters
  std::vector<double> generateSamples(const Synthesizable &params) const;
};

} // namespace synth
} // namespace MicroComposer

#endif
