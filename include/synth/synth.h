#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include "synth/concepts.h"
#include "synth/synth_output.h"
#include <cmath>
#include <vector>

namespace MicroComposer {
namespace synth {

class Synthesizer {
private:
  std::vector<double>::size_type sample_rate_;

protected:
  SynthOutput& output_;

  std::vector<double>::size_type
  compute_n_samples(const Synthesizable auto& params) const {
    return static_cast<std::vector<double>::size_type>(params.duration.count() *
                                                       sample_rate_);
  }

  // Generate audio samples for the given parameters
  std::vector<double> generateSamples(const Synthesizable auto& params) const {
    using size_t = std::vector<double>::size_type;
    static constexpr double TWO_PI = 2.0 * M_PI;

    std::vector<double> samples;
    const size_t n_samples = compute_n_samples(params);
    samples.reserve(n_samples);

    const double phase_increment = TWO_PI * params.frequency / sample_rate_;
    double phase = params.phase;

    for (size_t i = 0; i < n_samples; ++i) {
      samples.push_back(std::sin(phase));
      phase += phase_increment;
    }

    return samples;
  }

public:
  explicit Synthesizer(SynthOutput& output) : output_(output) {
    sample_rate_ = output.get_sample_rate();
  };

  // Generate and immediately output audio samples
  void play(const Synthesizable auto& params) const {
    auto samples = generateSamples(params);
    output_.write(samples);
  }
};

} // namespace synth
} // namespace MicroComposer

#endif
