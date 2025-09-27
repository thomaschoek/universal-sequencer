#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

struct Vector_event : public Event {
  std::vector<double> parameters;
};

static_assert(Sequencable<Vector_event>,
              "VectorEvent does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace Micro_composer

#endif
