#ifndef MICRO_COMPOSER_MUTABLE_EVENT_H
#define MICRO_COMPOSER_MUTABLE_EVENT_H

#include "sequencable/concepts.h"
#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

struct Mutable_event : public Event {
  // Default constructor required by Seq_event concept
  Mutable_event() = default;

  Mutable_event(Duration dur) : Event{dur} {}

  Mutable_event(Duration dur, Time_point time) : Event{dur, time} {}

  void set_duration(Duration dur) { duration = dur; }
  void set_scheduled_time(Time_point time) { scheduled_time = time; }

  void update(const Mutable_event& other) {
    duration = other.duration;
    scheduled_time = other.scheduled_time;
  }

  void update(Duration dur, Time_point time) {
    duration = dur;
    scheduled_time = time;
  }
};

static_assert(Mut_seq_event<Mutable_event>,
              "Mutable_event does not satisfy Mut_seq_event concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_H
