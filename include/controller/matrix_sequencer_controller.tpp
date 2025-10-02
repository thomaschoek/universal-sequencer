#include "matrix_sequencer_controller.h"
#include <stdexcept>

namespace Micro_composer {

namespace controller {

// Parameter selection methods

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_param(
    Param_idx param_idx) {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || !selected_step_idx_) {
    throw std::runtime_error(
        "[ERROR] In Matrix_sequencer_controller::select_param: No sequence "
        "or step selected.");
  }

  if (*selected_seq_idx_ >= Base_sequencer::size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer_controller::select_param: Sequencer "
        "index out of range.");
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (*selected_step_idx_ >= seq.size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer_controller::select_param: Step index "
        "out of range.");
  }

  auto step = seq.at(*selected_step_idx_); // Get copy for validation
  if (param_idx >= step.params.size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer_controller::select_param: Parameter "
        "index out of range.");
  }

  selected_param_idx_ = param_idx;
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_next_param() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || !selected_step_idx_) {
    return; // No sequence/step selected, can't navigate parameters
  }

  if (Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  auto step = seq.at(*selected_step_idx_); // Get copy
  if (step.params.empty()) {
    return;
  }

  if (!selected_param_idx_) {
    // No parameter selected, select first
    selected_param_idx_ = 0;
  } else {
    // Move to next parameter, wrap around
    selected_param_idx_ = (*selected_param_idx_ + 1) % step.params.size();
  }
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_prev_param() {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || !selected_step_idx_) {
    return; // No sequence/step selected, can't navigate parameters
  }

  if (Base_sequencer::empty()) {
    return;
  }

  auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
  if (seq.empty()) {
    return;
  }

  auto step = seq.at(*selected_step_idx_); // Get copy
  if (step.params.empty()) {
    return;
  }

  if (!selected_param_idx_) {
    // No parameter selected, select last
    selected_param_idx_ = step.params.size() - 1;
  } else {
    // Move to previous parameter, wrap around
    if (*selected_param_idx_ == 0) {
      selected_param_idx_ = step.params.size() - 1;
    } else {
      --(*selected_param_idx_);
    }
  }
}

template <typename T_event_params>
std::optional<typename Matrix_sequencer_controller<T_event_params>::Param_idx>
Matrix_sequencer_controller<T_event_params>::selected_param() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_param_idx_;
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::clear_param_selection() {
  std::scoped_lock lck{selection_mutex_};
  selected_param_idx_.reset();
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::update_selected(
    T_event_params&& value) {
  std::scoped_lock lck{selection_mutex_};

  if (!selected_seq_idx_ || !selected_step_idx_ || !selected_param_idx_) {
    throw std::runtime_error(
        "[ERROR] In Matrix_sequencer_controller::update_selected: No "
        "sequence, step, or parameter selected.");
  }

  // Use base class update method which properly handles the underlying data
  Base_sequencer::update(*selected_seq_idx_, *selected_step_idx_,
                         *selected_param_idx_,
                         std::forward<T_event_params>(value));
}

// Sequence and step selection methods

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select(Seq_idx seq_idx,
                                                         Step_idx step_idx) {
  std::scoped_lock lck{selection_mutex_};

  if (seq_idx >= Base_sequencer::size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer_controller::select: Sequencer index "
        "out of range.");
  }

  if (step_idx >= Base_sequencer::operator[](seq_idx).size()) {
    throw std::out_of_range(
        "[ERROR] In Matrix_sequencer_controller::select: Step index out of "
        "range.");
  }

  selected_seq_idx_ = seq_idx;
  selected_step_idx_ = step_idx;
  // Note: Don't clear param selection - allow it to persist across step changes
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_next_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select first sequencer
    selected_seq_idx_ = 0;
    // Only set step if sequence is not empty
    auto& seq = Base_sequencer::operator[](0);
    if (!seq.empty()) {
      selected_step_idx_ = 0;
    } else {
      selected_step_idx_.reset();
    }
  } else {
    // Move to next sequencer, wrap around
    selected_seq_idx_ = (*selected_seq_idx_ + 1) % Base_sequencer::size();
    // Reset step to 0 when changing sequencer (if not empty)
    auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
    if (!seq.empty()) {
      selected_step_idx_ = 0;
    } else {
      selected_step_idx_.reset();
    }
  }
  // Note: Don't clear param selection - allow it to persist across seq changes
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_prev_seq() {
  std::scoped_lock lck{selection_mutex_};

  if (Base_sequencer::empty()) {
    return;
  }

  if (!selected_seq_idx_) {
    // No selection, select last sequencer
    selected_seq_idx_ = Base_sequencer::size() - 1;
    // Only set step if sequence is not empty
    auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
    if (!seq.empty()) {
      selected_step_idx_ = 0;
    } else {
      selected_step_idx_.reset();
    }
  } else {
    // Move to previous sequencer, wrap around
    if (*selected_seq_idx_ == 0) {
      selected_seq_idx_ = Base_sequencer::size() - 1;
    } else {
      --(*selected_seq_idx_);
    }
    // Reset step to 0 when changing sequencer (if not empty)
    auto& seq = Base_sequencer::operator[](*selected_seq_idx_);
    if (!seq.empty()) {
      selected_step_idx_ = 0;
    } else {
      selected_step_idx_.reset();
    }
  }
  // Note: Don't clear param selection - allow it to persist across seq changes
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_next_step() {
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
  // Note: Don't clear param selection - allow it to persist across step changes
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::select_prev_step() {
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
  // Note: Don't clear param selection - allow it to persist across step changes
}

template <typename T_event_params>
std::optional<typename Matrix_sequencer_controller<T_event_params>::Seq_idx>
Matrix_sequencer_controller<T_event_params>::selected_seq() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_seq_idx_;
}

template <typename T_event_params>
std::optional<typename Matrix_sequencer_controller<T_event_params>::Step_idx>
Matrix_sequencer_controller<T_event_params>::selected_step() const {
  std::scoped_lock lck{selection_mutex_};
  return selected_step_idx_;
}

template <typename T_event_params>
void Matrix_sequencer_controller<T_event_params>::clear_selection() {
  std::scoped_lock lck{selection_mutex_};
  selected_seq_idx_.reset();
  selected_step_idx_.reset();
  selected_param_idx_.reset();
}

template <typename T_event_params>
gui::Display_state
Matrix_sequencer_controller<T_event_params>::get_display_state() const {
  gui::Display_state state;

  // Get selection state (thread-safe)
  {
    std::scoped_lock lck{selection_mutex_};
    state.selected_seq_idx = selected_seq_idx_;
    state.selected_step_idx = selected_step_idx_;
    state.selected_param_idx = selected_param_idx_;
  }

  // Get sequencer states
  // Note: We access the base class's transport_mutex_ for thread safety
  std::scoped_lock lck{Base_sequencer::transport_mutex_};

  for (Seq_idx i = 0; i < Base_sequencer::size(); ++i) {
    const auto& seq = Base_sequencer::operator[](i);

    gui::Sequencer_display_state seq_state;
    seq_state.current_step_idx = seq.get_pos();
    seq_state.is_running = seq.is_running();
    seq_state.num_steps = seq.size();

    // Get parameter values for each step
    if (seq_state.num_steps > 0) {
      // Determine number of parameters from first step
      auto first_step = seq.at(0);
      seq_state.num_params = first_step.params.size();

      // Get all step values
      for (Step_idx step_idx = 0; step_idx < seq_state.num_steps; ++step_idx) {
        auto step = seq.at(step_idx);
        gui::Step_display_state step_state;

        // Convert each parameter to string
        for (const auto& param : step.params) {
          step_state.param_values.push_back(std::to_string(param));
        }

        seq_state.steps.push_back(step_state);
      }
    } else {
      seq_state.num_params = 0;
    }

    // Expanded state: seq 0 starts expanded, others collapsed
    // (This should eventually be tracked per-sequence in the controller)
    seq_state.is_expanded = (i == state.selected_seq_idx.value_or(0));

    state.sequencers.push_back(seq_state);
  }

  return state;
}

} // namespace controller
} // namespace Micro_composer
