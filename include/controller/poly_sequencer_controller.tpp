#include "poly_sequencer_controller.h"
#include <stdexcept>

namespace Micro_composer {

namespace controller {

// Initialize state vectors from current sequencers
template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::init_state() {
  std::scoped_lock lck{state_mutex_};

  const auto n_seqs = Base_sequencer::size();
  state_.sizes.resize(n_seqs);
  state_.events.resize(n_seqs);
  state_.positions.resize(n_seqs);
  state_.scheduling.resize(n_seqs);
  state_.t_next.resize(n_seqs);

  for (Seq_idx i = 0; i < n_seqs; ++i) {
    const auto& seq = Base_sequencer::operator[](i);
    state_.sizes[i] = seq.size();
    state_.events[i] = seq.snapshot();
    state_.positions[i] = seq.get_pos();
    state_.scheduling[i] = seq.is_scheduling();
    state_.t_next[i] = seq.t_next();
  }
}

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
  std::scoped_lock lck{state_mutex_};
  State result = state_;

  // Query dynamic runtime state from each sequencer
  Seq_idx seq_index = 0;
  for (const auto& seq : *this) {
    result.positions[seq_index] = seq.get_pos();
    result.scheduling[seq_index] = seq.is_scheduling();
    result.t_next[seq_index] = seq.t_next();
    ++seq_index;
  }

  return result;
}

} // namespace controller
} // namespace Micro_composer
