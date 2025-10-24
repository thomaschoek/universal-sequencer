#ifndef MICRO_COMPOSER_PREMADE_SAMPLES_H
#define MICRO_COMPOSER_PREMADE_SAMPLES_H

#include "sequencable/oscillation_event.h"
#include <cmath>

namespace Micro_composer {

namespace sequencable {

struct Premade_samples : public Oscillation_event {
  std::vector<double> samples; // Audio samples

  // Default constructor
  Premade_samples() = default;

  explicit Premade_samples(double freq, double amp = 0.5, double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq, amp, ph},
        samples{generate_sine_wave(freq, amp, ph, duration, sample_rate)} {}

  explicit Premade_samples(double freq, double amp, double ph, Duration offset,
                           Duration duration)
      : Oscillation_event{freq, amp, ph, offset, duration},
        samples{
            generate_sine_wave(freq, amp, ph, duration, default_sample_rate)} {}

  void update(double freq = 440.0, double amp = 0.5, double ph = 0.0,
              double sample_rate = default_sample_rate) {
    frequency = freq;
    amplitude = amp;
    phase = ph;
    samples = generate_sine_wave(freq, amp, ph, duration, sample_rate);
  }

  static constexpr size_t default_sample_rate = 44100;

  static std::vector<double> generate_sine_wave(double frequency,
                                                double amplitude, double phase,
                                                Duration duration,
                                                double sample_rate) {
    using size_t = std::vector<double>::size_type;
    static constexpr double TWO_PI = 2.0 * M_PI;

    std::vector<double> samples;
    const size_t n_samples = static_cast<size_t>(
        std::chrono::duration<double>(duration).count() * sample_rate);
    samples.reserve(n_samples);

    const double phase_increment = TWO_PI * frequency / sample_rate;

    // Simple envelope to prevent clicks (fade in/out)
    const size_t fade_samples =
        std::min(n_samples / 20, 3 * size_t(sample_rate / frequency));
    const double amplitude_inc = amplitude / fade_samples;

    double current_phase = phase;
    double current_amplitude = 0.0;

    for (size_t i = 0; i < n_samples; ++i) {
      // Apply fade-in
      if (i < fade_samples) {
        current_amplitude += amplitude_inc;
      }
      // Apply fade-out
      else if (i >= n_samples - fade_samples) {
        current_amplitude -= amplitude_inc;
      } else {
        current_amplitude = amplitude;
      }

      double sample_value = current_amplitude * sin(current_phase);
      samples.push_back(sample_value);

      current_phase += phase_increment;
      if (current_phase >= TWO_PI) {
        current_phase -= TWO_PI;
      }
    }

    return samples;
  }
};

} // namespace sequencable

} // namespace Micro_composer
#endif
