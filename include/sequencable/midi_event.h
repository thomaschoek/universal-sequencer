#ifndef MICRO_COMPOSER_MIDI_EVENT_H
#define MICRO_COMPOSER_MIDI_EVENT_H

#include "sequencable/concepts.h"
#include "sequencable/mutable_event.h"
#include <libremidi/message.hpp>
#include <cstdint>
#include <string>

namespace Micro_composer {

namespace sequencable {

struct Midi_event : public Mutable_event {
  // MIDI message components
  uint8_t channel{0};     // MIDI channel (0-15)
  uint8_t note{60};       // Note number (0-127), 60 = Middle C
  uint8_t velocity{100};  // Note velocity (0-127)

  // Pair of note-on and note-off messages
  struct Messages {
    libremidi::message note_on;
    libremidi::message note_off;
  };

  // Default constructor required by Seq_event concept
  Midi_event() = default;

  // Constructor with duration and time
  Midi_event(Duration dur, Time_point time) : Mutable_event{dur, time} {}

  // Constructor with MIDI parameters
  Midi_event(Duration dur, uint8_t ch, uint8_t n, uint8_t vel)
      : Mutable_event{dur}, channel(ch), note(n), velocity(vel) {}

  // Constructor with MIDI parameters and time
  Midi_event(Duration dur, Time_point time, uint8_t ch, uint8_t n, uint8_t vel)
      : Mutable_event{dur, time}, channel(ch), note(n), velocity(vel) {}

  // Constructor from note name (e.g., "C4", "D#5")
  Midi_event(Duration dur, const std::string& note_name, uint8_t vel = 100);

  // Generate MIDI note-on and note-off messages
  Messages to_midi_messages() const;

  // Convert note number to name (e.g., 60 -> "C4")
  static std::string note_to_name(uint8_t note_num);

  // Convert note name to number (e.g., "C4" -> 60)
  static uint8_t name_to_note(const std::string& name);
};

static_assert(Mut_seq_event<Midi_event>,
              "Midi_event does not satisfy Mut_seq_event concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_MIDI_EVENT_H
