#ifndef MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H
#define MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H

#include "sequencable/concept.h"
#include "sequencer/multi_sequencer.h"

namespace Micro_composer {

namespace controller {

using sequencer::Multi_sequencer;

class Multi_sequence_controller : public Multi_sequencer {
public:
  using Base_t = Multi_sequencer;
  using Seq_idx = Base_t::Seqr_idx;
  using Step_idx = Base_t::Step_idx;

  void select_seq(Seq_idx);
  void select_seq_next();
  void select_seq_prev();
  void toggle_seq_current();
  void drop_seq_current();

  void select_step(Step_idx);
  void select_step_next();
  void select_step_prev();
  void set_step_current(sequencable::Sequencable auto&&);
  void toggle_step_current();
  void drop_step_current();

private:
  Seq_idx seq_idx_{0};
  Step_idx step_idx_{0};
};

} // namespace controller

} // namespace Micro_composer

#endif // MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H
