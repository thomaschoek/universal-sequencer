#ifndef MICRO_COMPOSER_EVENT_H
#define MICRO_COMPOSER_EVENT_H

#include "sequencable/sequencable.h"

namespace MicroComposer {

namespace sequencable {

struct Event {
  std::chrono::duration<double> offset{std::chrono::duration<double>(0)};
  std::chrono::duration<double> duration{std::chrono::duration<double>(1)};
};

static_assert(Sequencable<Event>, "Event does not satisfy Sequencable concept");

struct VectorEvent : public Event {
  std::vector<double> parameters;
};

static_assert(Sequencable<VectorEvent>,
              "VectorEvent does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace MicroComposer

#endif // MICRO_COMPOSER_EVENT_H
