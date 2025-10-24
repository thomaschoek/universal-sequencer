#ifndef MICRO_COMPOSER_PREMADE_SAMPLES_H
#define MICRO_COMPOSER_PREMADE_SAMPLES_H

#include "sequencable/oscillation_event.h"
#include <cassert>
#include <cmath>
#include <map>
#include <vector>

namespace Micro_composer {

namespace sequencable {

struct Premade_samples : public Oscillation_event {

  // Default constructor
  Premade_samples() = default;

  explicit Premade_samples(double freq, double amp = 0.5, double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq, amp, ph}, sample_rate_{sample_rate},
        samples_{generate_sine_wave(freq, amp, ph, duration, sample_rate)} {}

  explicit Premade_samples(const std::string& note_name, double amp = 0.5,
                           double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq_for_note(note_name), amp, ph},
        sample_rate_{sample_rate},
        samples_{generate_sine_wave(note_name, amp, ph, duration)} {}

  explicit Premade_samples(size_t note_number, double amp = 0.5,
                           double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq_for_note(note_number), amp, ph},
        sample_rate_{sample_rate},
        samples_{generate_sine_wave(note_number, amp, ph, duration)} {}

  explicit Premade_samples(double freq, double amp, double ph, Duration offset,
                           Duration duration)
      : Oscillation_event{freq, amp, ph, offset, duration},
        samples_{
            generate_sine_wave(freq, amp, ph, duration, default_sample_rate)} {}

  explicit Premade_samples(const std::string& note_name, double amp, double ph,
                           Duration offset, Duration duration)
      : Oscillation_event{freq_for_note(note_name), amp, ph, offset, duration},
        samples_{generate_sine_wave(note_name, amp, ph, duration)} {}

  void update(double freq = 440.0, double amp = 0.5, double ph = 0.0,
              double sample_rate = default_sample_rate) {
    frequency = freq;
    amplitude = amp;
    phase = ph;
    samples_ = generate_sine_wave(freq, amp, ph, duration, sample_rate);
  }

  static constexpr size_t default_sample_rate = 44100;

  static std::vector<double> generate_sine_wave(const Premade_samples& params) {
    return generate_sine_wave(params.frequency, params.amplitude, params.phase,
                              params.duration, params.sample_rate_);
  }

  static std::vector<double> generate_sine_wave(const std::string& note_name,
                                                double amplitude, double phase,
                                                Duration duration) {
    return generate_sine_wave(freq_for_note(note_name), amplitude, phase,
                              duration, default_sample_rate);
  }

  static std::vector<double> generate_sine_wave(size_t note_number,
                                                double amplitude, double phase,
                                                Duration duration) {
    return generate_sine_wave(freq_for_note(note_number), amplitude, phase,
                              duration, default_sample_rate);
  }

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

  static double freq_for_note(const std::string& name) {
    return freq_for_note(note_name_to_number(name));
  }

  static double note_name_to_number(const std::string& name) {
    // Vector of note frequencies from 1=A0 to 88=C8
    static const std::vector<double> freqs{
        27.50,   29.14,   30.87,   32.70,   34.65,   36.71,   38.89,   41.20,
        43.65,   46.25,   49.00,   51.91,   55.00,   58.27,   61.74,   65.41,
        69.30,   73.42,   77.78,   82.41,   87.31,   92.50,   98.00,   103.83,
        110.00,  116.54,  123.47,  130.81,  138.59,  146.83,  155.56,  164.81,
        174.61,  185.00,  196.00,  207.65,  220.00,  233.08,  246.94,  261.63,
        277.18,  293.66,  311.13,  329.63,  349.23,  369.99,  392.00,  415.30,
        440.00,  466.16,  493.88,  523.25,  554.37,  587.33,  622.25,  659.25,
        698.46,  739.99,  783.99,  830.61,  880.00,  932.33,  987.77,  1046.50,
        1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22,
        1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02, 2637.02,
        2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01,
        4434.92, 4698.63, 4978.03};

    assert(freqs.size() == 91);
    // Map of musical note names to frequencies (in Hz)
    static const std::map<std::string, double> notes{
        {"A0", freqs[0]},   {"A#0", freqs[1]},  {"B0", freqs[2]},
        {"C1", freqs[3]},   {"C#1", freqs[4]},  {"D1", freqs[5]},
        {"D#1", freqs[6]},  {"E1", freqs[7]},   {"F1", freqs[8]},
        {"F#1", freqs[9]},  {"G1", freqs[10]},  {"G#1", freqs[11]},
        {"A1", freqs[12]},  {"A#1", freqs[13]}, {"B1", freqs[14]},
        {"C2", freqs[15]},  {"C#2", freqs[16]}, {"D2", freqs[17]},
        {"D#2", freqs[18]}, {"E2", freqs[19]},  {"F2", freqs[20]},
        {"F#2", freqs[21]}, {"G2", freqs[22]},  {"G#2", freqs[23]},
        {"A2", freqs[24]},  {"A#2", freqs[25]}, {"B2", freqs[26]},
        {"C3", freqs[27]},  {"C#3", freqs[28]}, {"D3", freqs[29]},
        {"D#3", freqs[30]}, {"E3", freqs[31]},  {"F3", freqs[32]},
        {"F#3", freqs[33]}, {"G3", freqs[34]},  {"G#3", freqs[35]},
        {"A3", freqs[36]},  {"A#3", freqs[37]}, {"B3", freqs[38]},
        {"C4", freqs[39]},  {"C#4", freqs[40]}, {"D4", freqs[41]},
        {"D#4", freqs[42]}, {"E4", freqs[43]},  {"F4", freqs[44]},
        {"F#4", freqs[45]}, {"G4", freqs[46]},  {"G#4", freqs[47]},
        {"A4", freqs[48]},  {"A#4", freqs[49]}, {"B4", freqs[50]},
        {"C5", freqs[51]},  {"C#5", freqs[52]}, {"D5", freqs[53]},
        {"D#5", freqs[54]}, {"E5", freqs[55]},  {"F5", freqs[56]},
        {"F#5", freqs[57]}, {"G5", freqs[58]},  {"G#5", freqs[59]},
        {"A5", freqs[60]},  {"A#5", freqs[61]}, {"B5", freqs[62]},
        {"C6", freqs[63]},  {"C#6", freqs[64]}, {"D6", freqs[65]},
        {"D#6", freqs[66]}, {"E6", freqs[67]},  {"F6", freqs[68]},
        {"F#6", freqs[69]}, {"G6", freqs[70]},  {"G#6", freqs[71]},
        {"A6", freqs[72]},  {"A#6", freqs[73]}, {"B6", freqs[74]},
        {"C7", freqs[75]},  {"C#7", freqs[76]}, {"D7", freqs[77]},
        {"D#7", freqs[78]}, {"E7", freqs[79]},  {"F7", freqs[80]},
        {"F#7", freqs[81]}, {"G7", freqs[82]},  {"G#7", freqs[83]},
        {"A7", freqs[84]},  {"A#7", freqs[85]}, {"B7", freqs[86]},
        {"C8", freqs[87]}};
    assert(notes.size() == 88);
    return notes.at(name);
  }

  static double freq_for_note(size_t note) {
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 49.0) / 12.0);
  }

  double sample_rate_{default_sample_rate};
  // Pre-generated audio samples
  std::vector<double> samples_;
};

} // namespace sequencable

} // namespace Micro_composer
#endif
