#include "synth/synth.h"
#include <cmath>

namespace MicroComposer {
namespace synth {

inline double
Synthesizer::sine_sample(const double frequency,
                         const std::vector<double>::size_type phase) const {
  return std::sin(TWO_PI * (frequency / sample_rate_) * phase);
}

std::vector<double>
Synthesizer::sine_oscillation(const OscillationParams params) const {
  std::vector<double> samples(params.compute_n_samples(sample_rate_));
  for (auto i = samples.begin(); i != samples.end(); ++i) {
    const double d = std::distance(samples.begin(), i);
    *i = sine_sample(params.frequency, d);
  }
  return samples;
}

std::vector<double>
Synthesizer::square_oscillation(const OscillationParams params) const {
  std::vector<double> samples(params.compute_n_samples(sample_rate_));
  for (auto i = samples.begin(); i != samples.end(); ++i) {
    const double d = std::distance(samples.begin(), i);
    *i = square_sample(params.frequency, d);
  }
  return samples;
}

std::vector<double>
Synthesizer::generateSamples(const Synthesizable &params) const {
  const size_t num_samples =
      static_cast<size_t>(params.duration.count() * sample_rate_);
  std::vector<double> samples;
  samples.reserve(num_samples);

  for (size_t i = 0; i < num_samples; ++i) {
    const double time = static_cast<double>(i) / sample_rate_;
    double sample = 0.0;

    switch (params.waveform) {
    case WaveformType::SINE:
      sample = sine_sample(params);
      break;
    case WaveformType::SQUARE:
      sample = square_sample(params);
      break;
    case WaveformType::SAWTOOTH:
      sample = saw_sample(params);
      break;
    case WaveformType::TRIANGLE:
      sample = triangle_sample(params);
      break;
    }

    samples.push_back(sample * params.amplitude);
  }

  return samples;
}

} // namespace synth
} // namespace MicroComposer
