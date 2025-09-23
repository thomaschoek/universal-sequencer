#ifndef MICRO_COMPOSER_SYNTH_CONCEPTS_H
#define MICRO_COMPOSER_SYNTH_CONCEPTS_H

#include <chrono>

namespace Micro_composer {

namespace synth {

template <typename T>
concept Synthesizable = requires(T t) {
  { t.frequency } -> std::convertible_to<double>;
  { t.duration } -> std::convertible_to<std::chrono::duration<double>>;
};

} // namespace synth

} // namespace Micro_composer

#endif
