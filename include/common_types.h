#ifndef MICRO_COMPOSER_COMMON_TYPES_H
#define MICRO_COMPOSER_COMMON_TYPES_H

#include <chrono>

namespace Micro_composer {

struct Common_types {
  // Common time types
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
};

} // namespace Micro_composer

#endif // MICRO_COMPOSER_COMMON_TYPES_H
