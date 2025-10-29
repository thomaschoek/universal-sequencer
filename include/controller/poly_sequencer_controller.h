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
  using Handler_factory = Base_sequencer::Handler_factory;
  using Clock = Base_sequencer::Clock;
  using Time_point = Base_sequencer::Time_point;

  // Constructors
  Poly_sequencer_controller() = default;

  explicit Poly_sequencer_controller(const Handler_factory& factory,
                                    const std::vector<std::vector<Event_t>>& sequences = {})
      : Base_sequencer(factory, sequences) {
    init_state();
  }

  Poly_sequencer_controller(const std::vector<Handler>& handlers,
                           const std::vector<std::vector<Event_t>>& sequences)
      : Base_sequencer(handlers, sequences) {
    init_state();
  }

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

  // Event modification methods (wrap Base_sequencer methods to update state_)
  void update(Seq_idx seq, Event_idx pos, const Event_t& event);
  template <typename... Args>
  void update(Seq_idx seq, Event_idx pos, Args&&... args);

  void enable();
  void enable(Seq_idx);
  void enable(Seq_idx, Event_idx);
  void disable();
  void disable(Seq_idx);
  void disable(Seq_idx, Event_idx);
  void toggle();
  void toggle(Seq_idx);
  void toggle(Seq_idx, Event_idx);

  void adjust_durations(Seq_idx, typename Base_sequencer::Duration delta);
  void adjust_durations_all(typename Base_sequencer::Duration delta);

  void multiply_durations(Seq_idx, double factor);
  void multiply_durations_all(double factor);

  void for_each(Seq_idx, const std::function<void(Event_t&)>&);
  void for_each_all(const std::function<void(Event_t&)>&);

  void mutate(Seq_idx, Event_idx, const typename Base_sequencer::Sequencer_t::Mutator&);

  void replace(Seq_idx, Event_idx pos, const Event_t& event);
  void replace(Seq_idx, Event_idx start, const std::vector<Event_t>& events);

  // Add/remove events
  void push_back_event(Seq_idx seq, const Event_t& event);
  void pop_back_event(Seq_idx seq);

  struct State {
    std::optional<Seq_idx> selected_seq;
    std::optional<Event_idx> selected_event;
    std::vector<Event_idx> positions;
    std::vector<Event_idx> sizes;
    std::vector<bool> scheduling;
    std::vector<Time_point> t_next;
    std::vector<std::vector<Event_t>> events;
  };

  const State get_state() const;

private:
  void init_state();

  State state_;
  mutable std::mutex selection_mutex_;
  mutable std::mutex state_mutex_;
};

} // namespace controller
} // namespace Micro_composer

#include "poly_sequencer_controller.tpp"

#endif // MICRO_COMPOSER_POLY_SEQUENCER_CONTROLLER_H
