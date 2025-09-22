#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/event.h"

namespace MicroComposer {

namespace sequencable {

struct VectorEvent : public Event {
  std::vector<double> parameters;
};

static_assert(Sequencable<VectorEvent>,
              "VectorEvent does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace MicroComposer

#endif
