#include "poly_sequencer_controller.h"
#include <stdexcept>

namespace Micro_composer {

namespace controller {

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select(Seq_idx i_seq,
                                                Event_idx i_event) {
  std::scoped_lock lck{selection_mutex_};

  if (i_seq >= Base_sequencer::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer_controller::select: Sequencer index "
        "out of range.");
  }

  if (i_event >= Base_sequencer::operator[](i_seq).size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer_controller::select: Position index out of "
        "range.");
  }

  state_.selected_seq = i_seq;
  state_.selected_event = i_event;
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_next_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!state_.selected_seq) {
    // No selection, select first sequencer
    state_.selected_seq = 0;
    state_.selected_event = 0;
  } else {
    // Move to next sequencer, wrap around
    state_.selected_seq = (*state_.selected_seq + 1) % Base_sequencer::size();
    // Reset position to 0 when changing sequencer
    state_.selected_event = 0;
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_prev_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!state_.selected_seq) {
    // No selection, select last sequencer
    state_.selected_seq = Base_sequencer::size() - 1;
    state_.selected_event = 0;
  } else {
    // Move to previous sequencer, wrap around
    if (*state_.selected_seq == 0) {
      state_.selected_seq = Base_sequencer::size() - 1;
    } else {
      --(*state_.selected_seq);
    }
    // Reset position to 0 when changing sequencer
    state_.selected_event = 0;
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_next_pos() {
  std::scoped_lock lck{selection_mutex_};

  if (!state_.selected_seq || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*state_.selected_seq);
  if (seq.empty()) {
    return;
  }

  if (!state_.selected_event) {
    state_.selected_event = 0;
  } else {
    // Move to next position, wrap around
    state_.selected_event = (*state_.selected_event + 1) % seq.size();
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_prev_pos() {
  std::scoped_lock lck{selection_mutex_};

  if (!state_.selected_seq || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*state_.selected_seq);
  if (seq.empty()) {
    return;
  }

  if (!state_.selected_event) {
    state_.selected_event = seq.size() - 1;
  } else {
    // Move to previous position, wrap around
    if (*state_.selected_event == 0) {
      state_.selected_event = seq.size() - 1;
    } else {
      --(*state_.selected_event);
    }
  }
}

template <sequencable::Mut_seq_event T_event>
std::optional<typename Poly_sequencer_controller<T_event>::Seq_idx>
Poly_sequencer_controller<T_event>::selected_seq() const {
  std::scoped_lock lck{selection_mutex_};
  return state_.selected_seq;
}

template <sequencable::Mut_seq_event T_event>
std::optional<typename Poly_sequencer_controller<T_event>::Event_idx>
Poly_sequencer_controller<T_event>::selected_event() const {
  std::scoped_lock lck{selection_mutex_};
  return state_.selected_event;
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::clear_selection() {
  std::scoped_lock lck{selection_mutex_};
  state_.selected_seq.reset();
  state_.selected_event.reset();
}

template <sequencable::Mut_seq_event T_event>
const typename Poly_sequencer_controller<T_event>::State
Poly_sequencer_controller<T_event>::get_state() const {
  return state_;
}

} // namespace controller
} // namespace Micro_composer
