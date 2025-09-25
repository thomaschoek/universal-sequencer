#ifndef MICRO_COMPOSER_SEQUENCER_CONTROLLER_H
#define MICRO_COMPOSER_SEQUENCER_CONTROLLER_H

#include "concepts.h"
#include <memory>

namespace Micro_composer {

namespace controller {

template <Sequencer Sequencer_t> class Sequencer_controller {
public:
  Sequencer_controller();

  // Control methods that work with both sequencer types
  void start() { sequencer_->start(); }
  void stop() { sequencer_->stop(); }
  // etc.

private:
  std::unique_ptr<Sequencer_t> sequencer_;
  // Audio components, UI state, etc.
};

} // namespace controller

} // namespace Micro_composer

#endif // MICRO_COMPOSER_SEQUENCER_CONTROLLER_H
