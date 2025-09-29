#ifndef MICRO_COMPOSER_VECTOR_SEQUENCER_H
#define MICRO_COMPOSER_VECTOR_SEQUENCER_H

#include "sequencable/vector_event.h"
#include "sequencer/atomic_sequencer.h"

namespace Micro_composer {

namespace sequencer {

class Vector_sequencer : public Atomic_sequencer<sequencable::Vector_event> {
public:
  using Vector_event = sequencable::Vector_event;
  using Params = decltype(Vector_event::parameters);
  using Param_idx = Params::size_type;

private:
  using Base = Atomic_sequencer<Vector_event>;
};

} // namespace sequencer

} // namespace Micro_composer

#endif
