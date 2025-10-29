#ifndef MICRO_COMPOSER_TUPLE_EVENT_H
#define MICRO_COMPOSER_TUPLE_EVENT_H

#include "sequencable/mutable_event.h"
#include <tuple>

namespace Micro_composer {

namespace sequencable {

template <typename... Ts> class Tuple_event : public Mutable_event {
public:
  using Tuple = std::tuple<Ts...>;
  using Size_type = std::size_t;
  void update(const Tuple_event<Ts...>& other) { data = other.data; }
  void update(Size_type idx, auto value) { std::get<idx>(data) = value; }

  Tuple data;
};

} // namespace sequencable

} // namespace Micro_composer

#endif
