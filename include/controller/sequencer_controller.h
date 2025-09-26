#ifndef MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H
#define MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H

#include "sequencable/concept.h"
#include "sequencer/multi_sequencer.h"

namespace Micro_composer {

namespace controller {

using sequencable::Sequencable;
using sequencer::Multi_sequencer;

class Multi_sequence_controller : public Multi_sequencer {
public:
  using Base_t = Multi_sequencer;
  using Base_t::Seq_idx;
  using Base_t::Seq_ptr_t;
  using Base_t::Step_idx;

  template <Sequencable Event_t>
  void add_seq(std::function<void(Event_t&&)>,
               sequence::Atomic_step_sequence<Event_t>&& = {});
  void add_seq(Seq_ptr_t&&);
  void select_seq(Seq_idx);
  void select_seq_next();
  void select_seq_prev();
  void toggle_seq_current();
  void drop_seq(Seq_idx);
  void drop_seq_current();

  void push_step_back(Seq_idx, Sequencable auto&&);
  void push_step_front(Seq_idx, Sequencable auto&&);
  template <Sequencable Event_t> void pop_step_back(Seq_idx);
  template <Sequencable Event_t> void pop_step_front(Seq_idx);
  void select_step(Step_idx);
  void select_step_next();
  void select_step_prev();
  void set_step(Seq_idx, Step_idx, Sequencable auto&&);
  void set_step_current(sequencable::Sequencable auto&&);
  void toggle_step_current();
  void drop_step_current();

  void start_all(Sequencer_time_point = Sequencer_clock::now());
  void stop_all();
  void start(Seq_idx, Sequencer_time_point = Sequencer_clock::now());
  void stop(Seq_idx);

private:
  Seq_idx seq_idx_{0};
  Step_idx step_idx_{0};
};

} // namespace controller

} // namespace Micro_composer

#endif // MICRO_COMPOSER_MULTI_SEQUENCE_CONTROLLER_H
