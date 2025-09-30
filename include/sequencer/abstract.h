#ifndef MICRO_COMPOSER_SEQUENCER_INTERFACE_H
#define MICRO_COMPOSER_SEQUENCER_INTERFACE_H

#include <chrono>

namespace Micro_composer {

namespace sequencer {

namespace abstract {

class Sequencer {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point =
      std::chrono::time_point<Clock, std::chrono::duration<double>>;

  // Destructor
  virtual ~Sequencer() = default;

  // Transport control

  virtual bool is_running() const = 0;
  virtual void start(Time_point) = 0;
  virtual void stop() = 0;
};

} // namespace abstract

} // namespace sequencer

} // namespace Micro_composer

#endif
