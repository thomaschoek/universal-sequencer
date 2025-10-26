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
concept Seq_event = requires(T t) {
  { t.scheduled_time } -> std::convertible_to<Time_point>;
  { t.scheduled_time } -> std::assignable_from<Time_point>;
  { t.duration } -> std::convertible_to<Duration>;
  { t.duration } -> std::assignable_from<Duration>;
  requires std::default_initializable<T> && std::movable<T> &&
               std::destructible<T> && std::copyable<T>;
};

template <typename T>
concept Mut_seq_event = Seq_event<T> && requires(T t, const T& other) {
  // Require two update methods:
  // 1. update(const T&) - copy/assignment-style update
  { t.update(other) } -> std::same_as<void>;
  // 2. update(...) with arbitrary parameters - checked implicitly by overload existence
  // We just verify that 'update' is a member (at least one overload must exist)
};

} // namespace sequencable

} // namespace Micro_composer

#endif
