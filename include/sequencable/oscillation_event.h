#ifndef MICRO_COMPOSER_OSCILLATION_EVENT_H
#define MICRO_COMPOSER_OSCILLATION_EVENT_H

#include "sequencable/mutable_event.h"

namespace Micro_composer {

namespace sequencable {

struct Oscillation_event : public Mutable_event {
  Duration offset{std::chrono::milliseconds(0)};
  Duration duration{std::chrono::milliseconds(500)}; // Default 500 ms
  double frequency{440.0};                           // Frequency in Hz
  double amplitude{0.5};                             // Amplitude (0.0 to 1.0)
  double phase{0.0};                                 // Phase in radians

  // Default constructor
  Oscillation_event() = default;

  explicit Oscillation_event(double freq, double amp = 0.5, double ph = 0.0)
      : frequency(freq), amplitude(amp), phase(ph) {}

  explicit Oscillation_event(double freq, double amp, double ph,
                             Duration offset, Duration duration)
      : duration{duration}, offset(offset), frequency(freq), amplitude(amp),
        phase(ph) {}

  void update(double freq = 440.0, double amp = 0.5, double ph = 0.0) {
    frequency = freq;
    amplitude = amp;
    phase = ph;
  }

  void update(const Oscillation_event& other) {
    frequency = other.frequency;
    amplitude = other.amplitude;
    phase = other.phase;
    duration = other.duration;
    offset = other.offset;
  }

  // Comparison operators (compare event parameters, not scheduled_time)
  bool operator==(const Oscillation_event& other) const {
    return frequency == other.frequency && amplitude == other.amplitude &&
           phase == other.phase && duration == other.duration &&
           offset == other.offset;
  }

  bool operator!=(const Oscillation_event& other) const {
    return !(*this == other);
  }
};

static_assert(Mut_seq_event<Oscillation_event>,
              "Oscillation_event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif
