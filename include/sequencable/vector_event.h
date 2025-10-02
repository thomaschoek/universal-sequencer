#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/parametrized_event.h"

namespace Micro_composer {

namespace sequencable {

template <typename T>
struct Vector_event : public Parametrized_event<std::vector<T>> {
  using Base_event = Parametrized_event<std::vector<T>>;
  using Size_type = typename std::vector<T>::size_type;

  // Satisfies Sequencable_updatable concept - generic update for all params
  template <typename... Args>
  void update(Args&&... args) {
    Base_event::params = std::vector<T>{std::forward<Args>(args)...};
  }

  // Specific update for single parameter at index
  void update(Size_type idx, T&& value) {
    Base_event::params.at(idx) = std::forward<T>(value);
  }
};

} // namespace sequencable

} // namespace Micro_composer

#endif
