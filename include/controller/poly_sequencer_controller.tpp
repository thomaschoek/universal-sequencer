#include "poly_sequencer_controller.h"
#include <stdexcept>

namespace Micro_composer {

namespace controller {

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select(Seq_idx seq_idx,
                                                Pos_idx pos_idx) {
  std::scoped_lock lck{selection_mutex_};

  if (seq_idx >= Base_sequencer::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer_controller::select: Sequencer index "
        "out of range.");
  }

  if (pos_idx >= Base_sequencer::operator[](seq_idx).size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer_controller::select: Position index out of "
        "range.");
  }

  selected_seq_idx_ = seq_idx;
  selected_pos_idx_ = pos_idx;
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_next_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select first sequencer
    selected_seq_idx_ = 0;
    selected_pos_idx_ = 0;
  } else {
    // Move to next sequencer, wrap around
    selected_seq_idx_ = (*selected_seq_idx_ + 1) % Base_sequencer::size();
    // Reset position to 0 when changing sequencer
    selected_pos_idx_ = 0;
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_prev_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select last sequencer
    selected_seq_idx_ = Base_sequencer::size() - 1;
    selected_pos_idx_ = 0;
  } else {
    // Move to previous sequencer, wrap around
    if (*selected_seq_idx_ == 0) {
      selected_seq_idx_ = Base_sequencer::size() - 1;
    } else {
      --(*selected_seq_idx_);
    }
    // Reset position to 0 when changing sequencer
    selected_pos_idx_ = 0;
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_next_pos() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  if (!selected_pos_idx_) {
    selected_pos_idx_ = 0;
  } else {
    // Move to next position, wrap around
    selected_pos_idx_ = (*selected_pos_idx_ + 1) % seq.size();
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::select_prev_pos() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  if (!selected_pos_idx_) {
    selected_pos_idx_ = seq.size() - 1;
  } else {
    // Move to previous position, wrap around
    if (*selected_pos_idx_ == 0) {
      selected_pos_idx_ = seq.size() - 1;
    } else {
      --(*selected_pos_idx_);
    }
  }
}

template <sequencable::Mut_seq_event T_event>
std::optional<typename Poly_sequencer_controller<T_event>::Seq_idx>
Poly_sequencer_controller<T_event>::selected_seq() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_seq_idx_;
}

template <sequencable::Mut_seq_event T_event>
std::optional<typename Poly_sequencer_controller<T_event>::Pos_idx>
Poly_sequencer_controller<T_event>::selected_pos() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_pos_idx_;
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer_controller<T_event>::clear_selection() {
  std::scoped_lock lck{selection_mutex_};
  selected_seq_idx_.reset();
  selected_pos_idx_.reset();
}

} // namespace controller
} // namespace Micro_composer
