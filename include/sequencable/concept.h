#ifndef MICRO_COMPOSER_SEQUENCABLE_H
#define MICRO_COMPOSER_SEQUENCABLE_H

#include <chrono>
#include <concepts>

namespace MicroComposer {

namespace sequencable {

template <typename T>
concept Sequencable = requires(T t) {
  { t.duration } -> std::convertible_to<std::chrono::duration<double>>;
  { t.offset } -> std::convertible_to<std::chrono::duration<double>>;
} && std::default_initializable<T> && std::movable<T>;

} // namespace sequencable

} // namespace MicroComposer

#endif
