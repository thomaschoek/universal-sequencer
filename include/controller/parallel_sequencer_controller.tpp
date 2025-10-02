#include "parallel_sequencer_controller.h"
#include <stdexcept>

namespace Micro_composer {

namespace controller {

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::select(Seq_idx seq_idx,
                                                    Step_idx step_idx) {
  std::scoped_lock lck{selection_mutex_};

  if (seq_idx >= Base_sequencer::size()) {
    throw std::out_of_range(
        "[ERROR] In Parallel_sequencer_controller::select: Sequencer index "
        "out of range.");
  }

  if (step_idx >= Base_sequencer::operator[](seq_idx).size()) {
    throw std::out_of_range(
        "[ERROR] In Parallel_sequencer_controller::select: Step index out of "
        "range.");
  }

  selected_seq_idx_ = seq_idx;
  selected_step_idx_ = step_idx;
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::select_next_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select first sequencer
    selected_seq_idx_ = 0;
    selected_step_idx_ = 0;
  } else {
    // Move to next sequencer, wrap around
    selected_seq_idx_ = (*selected_seq_idx_ + 1) % Base_sequencer::size();
    // Reset step to 0 when changing sequencer
    selected_step_idx_ = 0;
  }
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::select_prev_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select last sequencer
    selected_seq_idx_ = Base_sequencer::size() - 1;
    selected_step_idx_ = 0;
  } else {
    // Move to previous sequencer, wrap around
    if (*selected_seq_idx_ == 0) {
      selected_seq_idx_ = Base_sequencer::size() - 1;
    } else {
      --(*selected_seq_idx_);
    }
    // Reset step to 0 when changing sequencer
    selected_step_idx_ = 0;
  }
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::select_next_step() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  if (!selected_step_idx_) {
    selected_step_idx_ = 0;
  } else {
    // Move to next step, wrap around
    selected_step_idx_ = (*selected_step_idx_ + 1) % seq.size();
  }
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::select_prev_step() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  if (!selected_step_idx_) {
    selected_step_idx_ = seq.size() - 1;
  } else {
    // Move to previous step, wrap around
    if (*selected_step_idx_ == 0) {
      selected_step_idx_ = seq.size() - 1;
    } else {
      --(*selected_step_idx_);
    }
  }
}

template <sequencable::Sequencable_updatable Event_t>
std::optional<typename Parallel_sequencer_controller<Event_t>::Seq_idx>
Parallel_sequencer_controller<Event_t>::selected_seq() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_seq_idx_;
}

template <sequencable::Sequencable_updatable Event_t>
std::optional<typename Parallel_sequencer_controller<Event_t>::Step_idx>
Parallel_sequencer_controller<Event_t>::selected_step() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_step_idx_;
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer_controller<Event_t>::clear_selection() {
  std::scoped_lock lck{selection_mutex_};
  selected_seq_idx_.reset();
  selected_step_idx_.reset();
}

} // namespace controller
} // namespace Micro_composer