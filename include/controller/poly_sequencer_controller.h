#ifndef MICRO_COMPOSER_POLY_SEQUENCER_CONTROLLER_H
#define MICRO_COMPOSER_POLY_SEQUENCER_CONTROLLER_H

#include "sequencable/concepts.h"
#include "sequencer/poly_sequencer_template.h"
#include <optional>

namespace Micro_composer {

namespace controller {

template <sequencable::Mut_seq_event Event_t>
class Poly_sequencer_controller : public sequencer::Poly_sequencer<Event_t> {
public:
  using Base_sequencer = sequencer::Poly_sequencer<Event_t>;
  using Sequencer_t = typename Base_sequencer::Sequencer_t;
  using Seq_idx = typename Base_sequencer::Seq_idx;
  using Handler = typename Base_sequencer::Handler;
  using Clock = typename Base_sequencer::Clock;
  using Time_point = typename Base_sequencer::Time_point;
  using Pos_idx = size_t;

  // Constructors - inherit from Poly_sequencer
  using Base_sequencer::Base_sequencer;

  // Selection management
  void select(Seq_idx seq_idx, Pos_idx pos_idx = 0);
  void select_next_seq();
  void select_prev_seq();
  void select_next_pos();
  void select_prev_pos();

  // Query selection
  std::optional<Seq_idx> selected_seq() const;
  std::optional<Pos_idx> selected_pos() const;

  // Clear selection
  void clear_selection();

private:
  std::optional<Seq_idx> selected_seq_idx_;
  std::optional<Pos_idx> selected_pos_idx_;
  mutable std::mutex selection_mutex_;
};

} // namespace controller
} // namespace Micro_composer

#include "poly_sequencer_controller.tpp"

#endif // MICRO_COMPOSER_POLY_SEQUENCER_CONTROLLER_H
