#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/event.h"

namespace Micro_composer {

namespace sequencable {

template <typename T> struct Vector_event : public Event {
  std::vector<T> parameters;
  void update(size_t idx, T&& value) { parameters.at(idx, std::move(value)); };
};

} // namespace sequencable

} // namespace Micro_composer

#endif
