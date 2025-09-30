#ifndef MICRO_COMPOSER_PARALLEL_SEQUENCER_CONTROLLER_H
#define MICRO_COMPOSER_PARALLEL_SEQUENCER_CONTROLLER_H

#include "sequencer/parallel_sequencer.h"
#include <optional>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable_updatable Event_t>
class Parallel_sequencer_controller
    : public Parallel_sequencer<Event_t> {
public:
  using Base_sequencer = Parallel_sequencer<Event_t>;
  using Sequencer = typename Base_sequencer::Sequencer;
  using Seq_idx = typename Base_sequencer::Seq_idx;
  using Step_idx = typename Sequencer::Step_idx;
  using Handler = typename Base_sequencer::Handler;
  using Clock = typename Base_sequencer::Clock;
  using Time_point = typename Base_sequencer::Time_point;

  // Constructors - inherit from Parallel_sequencer
  using Base_sequencer::Base_sequencer;

  // Selection management
  void select(Seq_idx seq_idx, Step_idx step_idx = 0);
  void select_next_seq();
  void select_prev_seq();
  void select_next_step();
  void select_prev_step();

  // Query selection
  std::optional<Seq_idx> selected_seq() const;
  std::optional<Step_idx> selected_step() const;

  // Clear selection
  void clear_selection();

private:
  std::optional<Seq_idx> selected_seq_idx_;
  std::optional<Step_idx> selected_step_idx_;
  mutable std::mutex selection_mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/parallel_sequencer_controller.tpp"

#endif // MICRO_COMPOSER_PARALLEL_SEQUENCER_CONTROLLER_H