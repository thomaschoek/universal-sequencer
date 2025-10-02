#ifndef MICRO_COMPOSER_DISPLAY_STATE_H
#define MICRO_COMPOSER_DISPLAY_STATE_H

#include <cstddef>
#include <optional>
#include <vector>

namespace Micro_composer {

namespace gui {

// Represents the current state of a single sequencer for display purposes
struct Sequencer_display_state {
  std::size_t current_step_idx;  // Current step being played (or last played)
  bool is_running;
  std::size_t num_steps;

  bool operator==(const Sequencer_display_state& other) const {
    return current_step_idx == other.current_step_idx &&
           is_running == other.is_running && num_steps == other.num_steps;
  }

  bool operator!=(const Sequencer_display_state& other) const {
    return !(*this == other);
  }
};

// Represents the full display state for the GUI
struct Display_state {
  // State of all sequencers
  std::vector<Sequencer_display_state> sequencers;

  // Current selection
  std::optional<std::size_t> selected_seq_idx;
  std::optional<std::size_t> selected_step_idx;
  std::optional<std::size_t> selected_param_idx;

  bool operator==(const Display_state& other) const {
    return sequencers == other.sequencers &&
           selected_seq_idx == other.selected_seq_idx &&
           selected_step_idx == other.selected_step_idx &&
           selected_param_idx == other.selected_param_idx;
  }

  bool operator!=(const Display_state& other) const { return !(*this == other); }
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_DISPLAY_STATE_H
