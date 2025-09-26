#ifndef MICRO_COMPOSER_EVENT_H
#define MICRO_COMPOSER_EVENT_H

#include "sequencable/concept.h"

namespace Micro_composer {

namespace sequencable {

struct Event {
  std::chrono::duration<double> offset{std::chrono::duration<double>(0.0)};
  std::chrono::duration<double> duration{std::chrono::duration<double>(0.25)};
  bool enabled;
};

static_assert(Sequencable<Event>, "Event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_H
