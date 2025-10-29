#ifndef MICRO_COMPOSER_MIDI_EVENT_H
#define MICRO_COMPOSER_MIDI_EVENT_H

#include "sequencable/concepts.h"
#include "sequencable/mutable_event.h"

namespace Micro_composer {

namespace sequencable {

struct Midi_event : public Mutable_event {
  // Default constructor required by Seq_event concept
  Midi_event() = default;

  Midi_event(Duration dur, Time_point time) : Mutable_event{dur, time} {}
};

static_assert(Mut_seq_event<Midi_event>,
              "Midi_event does not satisfy Mut_seq_event concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_H
