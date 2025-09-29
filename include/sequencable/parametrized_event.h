#ifndef MICRO_COMPOSER_PARAMETRIZED_EVENT_H
#define MICRO_COMPOSER_PARAMETRIZED_EVENT_H

#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

template <typename Parameters> struct Parametrized_event : public Event {
  Parameters params;
};

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_PARAMETRIZED_EVENT_H
