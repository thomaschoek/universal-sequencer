#include "sequencer_controller.h"

namespace Micro_composer {

namespace controller {

template <Sequencer Sequencer_t>
Sequencer_controller<Sequencer_t>::Sequencer_controller() {

  sequencer_ = std::make_unique<Sequencer_t>();
}

} // namespace controller

} // namespace Micro_composer
