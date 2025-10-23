#ifndef MICRO_COMPOSER_SEQUENCABLE_H
#define MICRO_COMPOSER_SEQUENCABLE_H

#include <concepts>

#include "common_types.h"

namespace Micro_composer {

namespace sequencable {

using Clock = Common_types::Clock;
using Time_point = Common_types::Time_point;
using Duration = Common_types::Duration;

template <typename T>
concept Sequencable = requires(T t) {
  { t.scheduled_time } -> std::convertible_to<Time_point>;
  { t.scheduled_time } -> std::assignable_from<Time_point>;
  { t.duration } -> std::convertible_to<Duration>;
  { t.duration } -> std::assignable_from<Duration>;
  requires std::default_initializable<T> && std::movable<T> &&
               std::destructible<T> && std::copyable<T>;
};

template <typename T>
concept Sequencable_parametrized_updatable = Sequencable<T> && requires(T t) {
  { t.parameters } -> std::assignable_from<decltype(t.parameters)>;
};

template <typename T>
concept Vector_sequencable = Sequencable<T> && requires(T t) {
  { t[0] } -> std::convertible_to<double>;
};

template <typename T, typename... Args>
concept Sequencable_updatable =
    Sequencable<T> && requires(T t, Args... args) { t.update(args...); };

} // namespace sequencable

} // namespace Micro_composer

#endif
