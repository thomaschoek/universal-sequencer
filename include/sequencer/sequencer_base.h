#ifndef MICRO_COMPOSER_SEQUENCER_BASE_H
#define MICRO_COMPOSER_SEQUENCER_BASE_H

#include <chrono>

namespace Micro_composer {

namespace sequencer {

class Sequencer_base {
public:
  using clock = std::chrono::steady_clock;
  using time_point = typename clock::time_point;

  virtual ~Sequencer_base() noexcept = default;
  virtual void start(time_point) = 0;
  virtual void stop() = 0;
  virtual bool is_running() const = 0;
};

} // namespace sequencer

} // namespace Micro_composer

#endif
