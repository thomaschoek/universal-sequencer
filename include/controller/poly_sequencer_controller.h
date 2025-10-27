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
  using Sequencer_t = Base_sequencer::Sequencer_t;
  using Seq_idx = Base_sequencer::Seq_idx;
  using Event_idx = Base_sequencer::Event_idx;
  using Handler = Base_sequencer::Handler;
  using Clock = Base_sequencer::Clock;
  using Time_point = Base_sequencer::Time_point;

  // Constructors - inherit from Poly_sequencer
  using Base_sequencer::Base_sequencer;

  // Selection management
  void select(Seq_idx, Event_idx = 0);
  void select_next_seq();
  void select_prev_seq();
  void select_next_pos();
  void select_prev_pos();

  // Query selection
  std::optional<Seq_idx> selected_seq() const;
  std::optional<Event_idx> selected_event() const;

  // Clear selection
  void clear_selection();

  struct State {
    std::optional<Seq_idx> selected_seq;
    std::optional<Event_idx> selected_event;
    std::vector<Event_idx> positions;
    std::vector<Event_idx> sizes;
    std::vector<bool> scheduling;
    std::vector<std::vector<Event_t>> events;
  };

  const State get_state() const;

private:
  State state_;
  mutable std::mutex selection_mutex_;
};

} // namespace controller
} // namespace Micro_composer

#include "poly_sequencer_controller.tpp"

#endif // MICRO_COMPOSER_POLY_SEQUENCER_CONTROLLER_H
