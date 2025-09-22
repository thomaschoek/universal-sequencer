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
Synthesizer::generateSamples(const OscillationParams &params) const {
  std::vector<double> samples(params.compute_n_samples(sample_rate_));
  for (auto i = samples.begin(); i != samples.end(); ++i) {
    const double d = std::distance(samples.begin(), i);
    *i = sine_sample(params.frequency, d);
  }
  return samples;
}

void Synthesizer::play(const OscillationParams &params) const {
  auto samples = generateSamples(params);
  output_.write(samples);
}

} // namespace synth
} // namespace MicroComposer
