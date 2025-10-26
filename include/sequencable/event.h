#ifndef MICRO_COMPOSER_EVENT_H
#define MICRO_COMPOSER_EVENT_H

#include "sequencable/concepts.h"

namespace Micro_composer {

namespace sequencable {

struct Event {
  Duration duration;
  Time_point scheduled_time;
};

static_assert(Seq_event<Event>, "Event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_H
