#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/parametrized_event.h"

namespace Micro_composer {

namespace sequencable {

template <typename T>
struct Vector_event : public Parametrized_event<std::vector<T>> {
  using Base_event = Parametrized_event<std::vector<T>>;
  void update(size_t idx, T&& value) {
    Base_event::params.at(idx) = std::move(value);
  };
};

} // namespace sequencable

} // namespace Micro_composer

#endif
