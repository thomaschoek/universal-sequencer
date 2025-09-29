#ifndef MICRO_COMPOSER_PARAMETRIZED_EVENT_H
#define MICRO_COMPOSER_PARAMETRIZED_EVENT_H

#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

template <typename Parameters> struct Parametrized_event : public Event {
  Parametrized_event() : Event{}, params() {};
  Parametrized_event(decltype(Event::duration) duration,
                     decltype(Event::offset) offset)
      : Event{duration, offset}, params() {};
  template <typename... Args>
  Parametrized_event(decltype(Event::duration) duration,
                     decltype(Event::offset) offset, Args... args)
      : Event{duration, offset}, params(args...){};
  Parameters params;
};

} // namespace sequencable

} // namespace Micro_composer

#endif // MICRO_COMPOSER_PARAMETRIZED_EVENT_H
