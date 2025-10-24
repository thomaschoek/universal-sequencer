#include "synth/synth.h"
#include <cmath>
#ifndef NDEBUG
#include <iostream>
#include <syncstream>
#include <thread>
#endif

namespace Micro_composer {
namespace synth {

inline void debug_msg(std::string msg, std::ostream& stream = std::cerr) {
#ifndef NDEBUG
  std::osyncstream(stream) << "[SYNTHESIZER] thread "
                           << std::to_string(std::hash<std::thread::id>{}(
                                  std::this_thread::get_id()))
                           << ": " << msg << std::endl
                           << std::flush;
#endif
}

inline void Synthesizer::write(const std::vector<double>& samples) const {
  output_.write(samples);
}

// Generate and immediately output audio samples
template <Synthesizable T> void Synthesizer::play(const T& params) const {
  debug_msg("calling generate_samples(params)");
  auto samples = generate_samples(params);
  debug_msg("returned from generate_samples");
  debug_msg("calling output_.write(samples)");
  output_.write(samples);
}

template <Synthesizable T>
std::vector<double>::size_type
Synthesizer::compute_n_samples(const T& params) const {
  // Convert duration to seconds as a double before multiplying by sample_rate
  const double duration_seconds =
      std::chrono::duration<double>(params.duration).count();
  return static_cast<std::vector<double>::size_type>(duration_seconds *
                                                     sample_rate_);
}

template <Synthesizable T>
std::vector<double> Synthesizer::generate_samples(const T& params) const {
  using size_t = std::vector<double>::size_type;
  static constexpr double TWO_PI = 2.0 * M_PI;

  std::vector<double> samples;
  const size_t n_samples = compute_n_samples(params);
  samples.reserve(n_samples);

  const double phase_increment = TWO_PI * params.frequency / sample_rate_;

  // Simple envelope to prevent clicks (fade in/out)
  const size_t fade_samples =
      std::min(n_samples / 20, 3 * size_t(sample_rate_ / params.frequency));
  const double amplitude_inc = params.amplitude / fade_samples;

  double amplitude = amplitude_inc;
  double phase = params.phase;

  size_t i = 0;
  for (; i < fade_samples;
       ++i, amplitude += amplitude_inc, phase += phase_increment) {
    samples.push_back(amplitude * std::sin(phase));
  }

  for (; i < n_samples - fade_samples; ++i, phase += phase_increment) {
    samples.push_back(amplitude * std::sin(phase));
  }

  for (; i < n_samples;
       ++i, amplitude -= amplitude_inc, phase += phase_increment) {
    samples.push_back(amplitude * std::sin(phase));
  }

  return samples;
}

} // namespace synth
} // namespace Micro_composer