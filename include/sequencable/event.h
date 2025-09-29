#ifndef MICRO_COMPOSER_EVENT_H
#define MICRO_COMPOSER_EVENT_H

#include "sequencable/concepts.h"

namespace Micro_composer {

namespace sequencable {

struct Event {
  using Duration = std::chrono::duration<double>;
  Duration offset{std::chrono::duration<double>(0.0)};
  Duration duration{std::chrono::duration<double>(0.25)};
};

static_assert(Sequencable<Event>, "Event does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_H
