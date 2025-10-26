#ifndef MICRO_COMPOSER_VECTOR_EVENT_H
#define MICRO_COMPOSER_VECTOR_EVENT_H

#include "sequencable/mutable_event.h"

namespace Micro_composer {

namespace sequencable {

template <typename T> class Vector_event : public Mutable_event {
public:
  using Vector = std::vector<T>;
  using Size_type = Vector::size_type;
  void update(const Vector_event<T>& other) { data = other.data; }
  void update(Size_type idx, T value) { data.at(idx) = value; }

  Vector data;
};

} // namespace sequencable

} // namespace Micro_composer

#endif
