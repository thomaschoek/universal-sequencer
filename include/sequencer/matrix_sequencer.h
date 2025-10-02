#ifndef MICRO_COMPOSER_MATRIX_SEQUENCER_H
#define MICRO_COMPOSER_MATRIX_SEQUENCER_H

#include "sequencable/vector_event.h"
#include "sequencer/atomic_sequencer.h"
#include "sequencer/parallel_sequencer.h"
#include <initializer_list>
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace sequencer {

template <typename T_event_params>
class Matrix_sequencer
    : public Parallel_sequencer<sequencable::Vector_event<T_event_params>> {
public:
  using Vector_event = sequencable::Vector_event<T_event_params>;
  using Base = Parallel_sequencer<Vector_event>;
  using Sequencer = Base::Sequencer;
  using Base_vector = Base::Base_vector;
  using Seq_idx = Base_vector::size_type;
  using Step_idx = Sequencer::Step_idx;
  using Param_idx = typename Vector_event::Size_type;

  using Base::Parallel_sequencer;

  void update(Seq_idx, Step_idx, Param_idx, T_event_params&&);
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/matrix_sequencer.tpp"

#endif // MICRO_COMPOSER_MATRIX_SEQUENCER_H
