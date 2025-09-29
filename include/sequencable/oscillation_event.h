#ifndef MICRO_COMPOSER_OSCILLATION_EVENT_H
#define MICRO_COMPOSER_OSCILLATION_EVENT_H

#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

struct Oscillation_event : public Event {
  using Event_offset_t = decltype(Event::offset);
  using Event_duration_t = decltype(Event::duration);
  double frequency{440.0}; // Frequency in Hz
  double amplitude{0.5};   // Amplitude (0.0 to 1.0)
  double phase{0.0};       // Phase in radians

  // Default constructor
  Oscillation_event() = default;

  explicit Oscillation_event(double freq, double amp = 0.5, double ph = 0.0)
      : Event(), frequency(freq), amplitude(amp), phase(ph) {}

  explicit Oscillation_event(double freq, double amp, double ph,
                             Event_offset_t offset, Event_duration_t duration)
      : Event{offset, duration}, frequency(freq), amplitude(amp), phase(ph) {}

  void update(double freq = 440.0, double amp = 0.5, double ph = 0.0) {
    frequency = freq;
    amplitude = amp;
    phase = ph;
  }
};

static_assert(Sequencable_updatable<Oscillation_event>,
              "Oscillation_event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif
