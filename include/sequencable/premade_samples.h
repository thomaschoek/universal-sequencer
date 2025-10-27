#ifndef MICRO_COMPOSER_PREMADE_SAMPLES_H
#define MICRO_COMPOSER_PREMADE_SAMPLES_H

#include "sequencable/oscillation_event.h"
#include "utility/debug.h"
#include <cassert>
#include <cmath>
#include <map>
#include <vector>

namespace Micro_composer {

namespace sequencable {

struct Premade_samples : public Oscillation_event {

  // Default constructor
  Premade_samples() = default;

  explicit Premade_samples(size_t note_number, double amp = 0.5,
                           double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq_of(note_number), amp, ph},
        sample_rate_{sample_rate} {
    generate_samples();
  }

  explicit Premade_samples(size_t note_number, double amp, double ph,
                           Duration offset, Duration duration)
      : Oscillation_event{freqs_[note_number], amp, ph, offset, duration} {
    generate_samples();
  }
  explicit Premade_samples(const std::string& note_name, double amp = 0.5,
                           double ph = 0.0,
                           double sample_rate = default_sample_rate)
      : Oscillation_event{freq_of(note_name), amp, ph},
        sample_rate_{sample_rate} {
    generate_samples();
  }

  explicit Premade_samples(const std::string& note_name, double amp, double ph,
                           Duration offset, Duration duration)
      : Oscillation_event{freq_of(note_name), amp, ph, offset, duration} {
    generate_samples();
  }

  void set_duration(Duration dur) {
    duration = dur;
    generate_samples();
  }

  void update(size_t note_number = 39, double amp = 0.5, double ph = 0.0,
              double sample_rate = default_sample_rate) {
    frequency = freqs_[note_number];
    amplitude = amp;
    phase = ph;
    generate_samples();
  }

  void update(const Premade_samples& other) {
    Oscillation_event::update(other);
    generate_samples();
  }

  void generate_sine_wave(const std::string& note_name, double amplitude,
                          double phase, Duration duration) {
    frequency = freq_of(note_name);
    generate_samples();
  }

  //  static std::vector<double> generate_sine_wave(size_t note_number,
  //                                                double amplitude, double
  //                                                phase, Duration duration) {
  //    debug::msg(
  //        "generate_sine_wave(note_number= " + std::to_string(note_number) +
  //        ", frequency=" + std::to_string(freq_of(note_number)) + " Hz)");
  //    return generate_sine_wave(freq_of(note_number), amplitude, phase,
  //    duration,
  //                              default_sample_rate);
  //  }
  //
  void generate_samples() {
    using size_t = std::vector<double>::size_type;
    static constexpr double TWO_PI = 2.0 * M_PI;

    const size_t n_samples = static_cast<size_t>(
        std::chrono::duration<double>(duration).count() * sample_rate_);
    samples_.clear();
    samples_.reserve(n_samples);

    const double phase_increment = TWO_PI * frequency / sample_rate_;

    // Simple envelope to prevent clicks (fade in/out)
    const size_t fade_samples =
        std::min(n_samples / 20, 3 * size_t(sample_rate_ / frequency));
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

      samples_.push_back(sample_value);

      current_phase += phase_increment;
      if (current_phase >= TWO_PI) {
        current_phase -= TWO_PI;
      }
    }
  }

  static double freq_of(const std::string& name) {
    // Map of musical note names to frequencies (in Hz)
    static const std::map<std::string, double> notes{
        {"A0", freqs_[0]},   {"A#0", freqs_[1]},  {"B0", freqs_[2]},
        {"C1", freqs_[3]},   {"C#1", freqs_[4]},  {"D1", freqs_[5]},
        {"D#1", freqs_[6]},  {"E1", freqs_[7]},   {"F1", freqs_[8]},
        {"F#1", freqs_[9]},  {"G1", freqs_[10]},  {"G#1", freqs_[11]},
        {"A1", freqs_[12]},  {"A#1", freqs_[13]}, {"B1", freqs_[14]},
        {"C2", freqs_[15]},  {"C#2", freqs_[16]}, {"D2", freqs_[17]},
        {"D#2", freqs_[18]}, {"E2", freqs_[19]},  {"F2", freqs_[20]},
        {"F#2", freqs_[21]}, {"G2", freqs_[22]},  {"G#2", freqs_[23]},
        {"A2", freqs_[24]},  {"A#2", freqs_[25]}, {"B2", freqs_[26]},
        {"C3", freqs_[27]},  {"C#3", freqs_[28]}, {"D3", freqs_[29]},
        {"D#3", freqs_[30]}, {"E3", freqs_[31]},  {"F3", freqs_[32]},
        {"F#3", freqs_[33]}, {"G3", freqs_[34]},  {"G#3", freqs_[35]},
        {"A3", freqs_[36]},  {"A#3", freqs_[37]}, {"B3", freqs_[38]},
        {"C4", freqs_[39]},  {"C#4", freqs_[40]}, {"D4", freqs_[41]},
        {"D#4", freqs_[42]}, {"E4", freqs_[43]},  {"F4", freqs_[44]},
        {"F#4", freqs_[45]}, {"G4", freqs_[46]},  {"G#4", freqs_[47]},
        {"A4", freqs_[48]},  {"A#4", freqs_[49]}, {"B4", freqs_[50]},
        {"C5", freqs_[51]},  {"C#5", freqs_[52]}, {"D5", freqs_[53]},
        {"D#5", freqs_[54]}, {"E5", freqs_[55]},  {"F5", freqs_[56]},
        {"F#5", freqs_[57]}, {"G5", freqs_[58]},  {"G#5", freqs_[59]},
        {"A5", freqs_[60]},  {"A#5", freqs_[61]}, {"B5", freqs_[62]},
        {"C6", freqs_[63]},  {"C#6", freqs_[64]}, {"D6", freqs_[65]},
        {"D#6", freqs_[66]}, {"E6", freqs_[67]},  {"F6", freqs_[68]},
        {"F#6", freqs_[69]}, {"G6", freqs_[70]},  {"G#6", freqs_[71]},
        {"A6", freqs_[72]},  {"A#6", freqs_[73]}, {"B6", freqs_[74]},
        {"C7", freqs_[75]},  {"C#7", freqs_[76]}, {"D7", freqs_[77]},
        {"D#7", freqs_[78]}, {"E7", freqs_[79]},  {"F7", freqs_[80]},
        {"F#7", freqs_[81]}, {"G7", freqs_[82]},  {"G#7", freqs_[83]},
        {"A7", freqs_[84]},  {"A#7", freqs_[85]}, {"B7", freqs_[86]},
        {"C8", freqs_[87]}};
    assert(notes.size() == 88);
    return notes.at(name);
  }

  // Array of note frequencies from 0=A0 to 88=C8
  static constexpr double freqs_[91]{
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

  static double freq_of(size_t note) {
    if (note <= 90) {
      debug::msg("Using pre-defined frequency for note number " +
                 std::to_string(note));
      return freqs_[note];
    }
    {
      return 440.0 * std::pow(2.0, (static_cast<double>(note) - 49.0) / 12.0);
    }
  }

  static constexpr size_t default_sample_rate = 44100;
  double sample_rate_{default_sample_rate};
  // Pre-generated audio samples
  std::vector<double> samples_;
};

static_assert(Seq_event<Event>, "Event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer
#endif
