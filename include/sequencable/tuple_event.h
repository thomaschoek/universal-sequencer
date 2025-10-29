#ifndef MICRO_COMPOSER_TUPLE_EVENT_H
#define MICRO_COMPOSER_TUPLE_EVENT_H

#include "sequencable/mutable_event.h"
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Micro_composer {

namespace sequencable {

template <std::vector<std::string> param_labels,
          std::vector<std::string> param_units, typename... Ts>
class Tuple_event : public Mutable_event {
public:
  using Tuple = std::tuple<Ts...>;
  using Size_type = std::size_t;
  void update(const Tuple_event<Ts...>& other) { data = other.data; }
  void update(Size_type idx, auto value) { std::get<idx>(data) = value; }

  Tuple data;
  static constexpr std::vector<std::string> labels = param_labels;
  static constexpr std::vector<std::string> unints = param_units;
};

} // namespace sequencable

} // namespace Micro_composer

#endif
