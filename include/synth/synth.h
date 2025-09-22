#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include "synth/synth_output.h"
#include <chrono>
#include <cmath>
#include <vector>

namespace MicroComposer {
namespace synth {

struct OscillationParams {
  double frequency{440.0};
  std::chrono::duration<double> duration{1};
  double amplitude{0.5};
  double phase{0};
  std::vector<double>::size_type
  compute_n_samples(std::vector<double>::size_type sample_rate) const {
    return duration.count() * sample_rate;
  }
};

class Synthesizer {
private:
  static constexpr double TWO_PI = 2.0 * M_PI;
  std::vector<double>::size_type sample_rate_;
  inline double sine_sample(const double frequency,
                            const std::vector<double>::size_type phase) const;

protected:
  SynthOutput &output_;

  // Generate audio samples for the given parameters
  std::vector<double> generateSamples(const OscillationParams &params) const;

public:
  explicit Synthesizer(SynthOutput &output, const double sample_rate = 44100.0)
      : output_(output), sample_rate_(output.get_sample_rate()) {};

  // Generate and immediately output audio samples
  void play(const OscillationParams &params) const;
};

} // namespace synth
} // namespace MicroComposer

#endif
