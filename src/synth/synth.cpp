#include "synth/synth.h"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace MicroComposer {
namespace synth {

Synthesizer::Synthesizer(double sample_rate) : sample_rate_(sample_rate) {}

void Synthesizer::synthesize(const Synthesizable &params, WaveformType waveform) {
  auto samples = generateSamples(params, waveform);

  if (audio_callback_) {
    audio_callback_(samples, sample_rate_);
  } else {
    // Default behavior: output basic info to console
    std::cout << "[SYNTH] Playing: " << std::fixed << std::setprecision(2)
              << params.frequency << "Hz, "
              << params.amplitude << " amplitude, "
              << params.duration.count() << "s ("
              << samples.size() << " samples)" << std::endl;
  }
}

void Synthesizer::setAudioCallback(std::function<void(const std::vector<double>&, double)> callback) {
  audio_callback_ = callback;
}

std::vector<double> Synthesizer::generateSamples(const Synthesizable &params, WaveformType waveform) const {
  const size_t num_samples = static_cast<size_t>(params.duration.count() * sample_rate_);
  std::vector<double> samples;
  samples.reserve(num_samples);

  for (size_t i = 0; i < num_samples; ++i) {
    const double time = static_cast<double>(i) / sample_rate_;
    double sample = 0.0;

    switch (waveform) {
      case WaveformType::SINE:
        sample = generateSine(params.frequency, params.phase, time);
        break;
      case WaveformType::SQUARE:
        sample = generateSquare(params.frequency, params.phase, time);
        break;
      case WaveformType::SAWTOOTH:
        sample = generateSawtooth(params.frequency, params.phase, time);
        break;
      case WaveformType::TRIANGLE:
        sample = generateTriangle(params.frequency, params.phase, time);
        break;
    }

    samples.push_back(sample * params.amplitude);
  }

  return samples;
}

double Synthesizer::generateSine(double frequency, double phase, double time) const {
  return std::sin(2.0 * M_PI * frequency * time + phase);
}

double Synthesizer::generateSquare(double frequency, double phase, double time) const {
  const double sine_value = generateSine(frequency, phase, time);
  return (sine_value >= 0.0) ? 1.0 : -1.0;
}

double Synthesizer::generateSawtooth(double frequency, double phase, double time) const {
  const double period = 1.0 / frequency;
  const double phase_time = std::fmod(time + phase / (2.0 * M_PI * frequency), period);
  return 2.0 * (phase_time / period) - 1.0;
}

double Synthesizer::generateTriangle(double frequency, double phase, double time) const {
  const double period = 1.0 / frequency;
  const double phase_time = std::fmod(time + phase / (2.0 * M_PI * frequency), period);
  const double normalized_time = phase_time / period;

  if (normalized_time < 0.5) {
    return 4.0 * normalized_time - 1.0;  // Rising edge
  } else {
    return 3.0 - 4.0 * normalized_time;  // Falling edge
  }
}

} // namespace synth
} // namespace MicroComposer
