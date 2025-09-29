#ifndef MICRO_COMPOSER_SEQUENCABLE_H
#define MICRO_COMPOSER_SEQUENCABLE_H

#include <chrono>
#include <concepts>

namespace Micro_composer {

namespace sequencable {

template <typename T>
concept Sequencable =
    requires(T t) {
      {
        t.duration, t.offset
      } -> std::convertible_to<std::chrono::duration<double>>;
      {
        t.duration, t.offset
      } -> std::assignable_from<std::chrono::duration<double>>;
    } && std::default_initializable<T> && std::movable<T> &&
    std::destructible<T> && std::copyable<T>;

template <typename T>
concept Sequencable_parametrized_updatable = Sequencable<T> && requires(T t) {
  { t.parameters } -> std::assignable_from<decltype(t.parameters)>;
};

template <typename T>
concept Vector_sequencable = Sequencable<T> && requires(T t) {
  { t[0] } -> std::convertible_to<double>;
};

template <typename T, typename... Args>
concept Sequencable_updatable = Sequencable<T> && requires(T t, Args... args) {
  t.update(args...);
};

} // namespace sequencable

} // namespace Micro_composer

#endif
