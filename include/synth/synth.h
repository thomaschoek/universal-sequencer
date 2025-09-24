#ifndef MICRO_COMPOSER_SYNTH_H
#define MICRO_COMPOSER_SYNTH_H

#include "synth/concepts.h"
#include "synth/synth_output.h"
#include <vector>

namespace Micro_composer {
namespace synth {

class Synthesizer {
public:
  explicit Synthesizer(SynthOutput& output) : output_(output) {
    sample_rate_ =
        static_cast<std::vector<double>::size_type>(output.get_sample_rate());
  }

  void write(const std::vector<double>& samples) const;

  // Generate and immediately output audio samples
  void play(const Synthesizable auto& params) const;

protected:
  std::vector<double>::size_type
  compute_n_samples(const Synthesizable auto& params) const;
  // Generate audio samples for the given parameters
  std::vector<double> generate_samples(const Synthesizable auto& params) const;

private:
  std::vector<double>::size_type sample_rate_;
  SynthOutput& output_;
};

} // namespace synth
} // namespace Micro_composer

#endif
