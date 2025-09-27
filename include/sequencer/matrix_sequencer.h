#ifndef MICRO_COMPOSER_MATRIX_SEQUENCER_H
#define MICRO_COMPOSER_MATRIX_SEQUENCER_H

#include "sequencable/vector_event.h"
#include "sequencer/atomic_sequencer.h"

namespace Micro_composer {

namespace sequencer {

class Matrix_sequencer {
public:
  using Vector_event = sequencable::Vector_event;
  using Sequencer_t = Atomic_sequencer<Vector_event>;
  using Handler_t = Sequencer_t::Handler_t;
  using Sequence_t = Sequencer_t::Sequence_t;
  using Clock = Sequencer_t::Clock;
  using Time_point = Sequencer_t::Time_point;
  using Sequencer_vec = std::vector<Sequencer_t>;
  using Seq_idx = Sequencer_vec::size_type;

  void start(Seq_idx, Time_point = Clock::now());
  void start_all(Time_point = Clock::now());
  void stop(Seq_idx);
  void stop_all();

private:
  Sequencer_vec sequencers_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_MATRIX_SEQUENCER_H
