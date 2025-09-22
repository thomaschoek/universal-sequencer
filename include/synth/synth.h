#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include "synth/concepts.h"
#include "synth/synth_output.h"
#include <vector>
#include <cmath>

namespace MicroComposer {
namespace synth {

class Synthesizer {
private:
  std::vector<double>::size_type sample_rate_;
  inline double sine_sample(const double frequency,
                            const std::vector<double>::size_type phase) const {
    static constexpr double TWO_PI = 2.0 * M_PI;
    return std::sin(TWO_PI * (frequency / sample_rate_) * phase);
  }

protected:
  SynthOutput &output_;

  std::vector<double>::size_type
  compute_n_samples(const Synthesizable auto &params) const {
    return static_cast<std::vector<double>::size_type>(params.duration.count() *
                                                       sample_rate_);
  }

  // Generate audio samples for the given parameters
  std::vector<double> generateSamples(const Synthesizable auto &params) const {
    std::vector<double> samples(compute_n_samples(params));
    for (auto i = samples.begin(); i != samples.end(); ++i) {
      const double d = std::distance(samples.begin(), i);
      *i = sine_sample(params.frequency, d);
    }
    return samples;
  }

public:
  explicit Synthesizer(SynthOutput &output) : output_(output) {
    sample_rate_ = output.get_sample_rate();
  };

  // Generate and immediately output audio samples
  void play(const Synthesizable auto &params) const {
    auto samples = generateSamples(params);
    output_.write(samples);
  }
};

} // namespace synth
} // namespace MicroComposer

#endif
