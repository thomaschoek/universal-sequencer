#ifndef MICRO_COMPOSER_MATRIX_SEQUENCER_CONTROLLER_H
#define MICRO_COMPOSER_MATRIX_SEQUENCER_CONTROLLER_H

#include "sequencer/matrix_sequencer.h"
#include <mutex>
#include <optional>

namespace Micro_composer {

namespace controller {

template <typename T_event_params>
class Matrix_sequencer_controller
    : public sequencer::Matrix_sequencer<T_event_params> {
public:
  using Base_sequencer = sequencer::Matrix_sequencer<T_event_params>;
  using Vector_event = typename Base_sequencer::Vector_event;
  using Sequencer = typename Base_sequencer::Sequencer;
  using Seq_idx = typename Base_sequencer::Seq_idx;
  using Step_idx = typename Base_sequencer::Step_idx;
  using Param_idx = typename Base_sequencer::Param_idx;

  // Constructors - inherit from Matrix_sequencer
  using Base_sequencer::Base_sequencer;

  // Parameter selection management
  void select_param(Param_idx param_idx);
  void select_next_param();
  void select_prev_param();

  // Query parameter selection
  std::optional<Param_idx> selected_param() const;

  // Clear parameter selection
  void clear_param_selection();

  // Update selected parameter
  void update_selected(T_event_params&& value);

  // Sequence and step selection (delegated to underlying sequencers)
  void select(Seq_idx seq_idx, Step_idx step_idx = 0);
  void select_next_seq();
  void select_prev_seq();
  void select_next_step();
  void select_prev_step();

  // Query sequence/step selection
  std::optional<Seq_idx> selected_seq() const;
  std::optional<Step_idx> selected_step() const;

  // Clear all selections
  void clear_selection();

private:
  std::optional<Seq_idx> selected_seq_idx_;
  std::optional<Step_idx> selected_step_idx_;
  std::optional<Param_idx> selected_param_idx_;
  mutable std::mutex selection_mutex_;
};

} // namespace controller
} // namespace Micro_composer

#include "matrix_sequencer_controller.tpp"

#endif // MICRO_COMPOSER_MATRIX_SEQUENCER_CONTROLLER_H
