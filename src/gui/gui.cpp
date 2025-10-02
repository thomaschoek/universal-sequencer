#include "gui/gui.h"
#include <iostream>

namespace Micro_composer {

namespace gui {

Gui::Gui() {
  // GTK will be initialized in init()
}

Gui::~Gui() {
  // GTK cleanup handled by gtk_main_quit if needed
}

void Gui::init(int argc, char** argv) {
  // Initialize GTK
  gtk_init(&argc, &argv);

  // Create main window
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window_), "Micro Composer");
  gtk_window_set_default_size(GTK_WINDOW(window_), 800, 600);

  // Connect destroy signal
  g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

  // Create grid for displaying sequencer steps
  grid_ = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid_), 5);
  gtk_grid_set_column_spacing(GTK_GRID(grid_), 5);

  // Add grid to window
  gtk_container_add(GTK_CONTAINER(window_), grid_);

  std::cout << "[INFO] GUI initialized" << std::endl;
}

void Gui::show() {
  if (window_) {
    gtk_widget_show_all(window_);
  }
}

void Gui::render(const Display_state& state) {
  // Skip rendering if state hasn't changed
  if (state == last_state_) {
    return;
  }

  std::cout << "[DEBUG] Rendering GUI update..." << std::endl;

  // For now, just print state to console
  // In a real implementation, this would update GTK widgets
  for (std::size_t i = 0; i < state.sequencers.size(); ++i) {
    const auto& seq_state = state.sequencers[i];
    std::cout << "  Seq[" << i << "]: step " << seq_state.current_step_idx
              << "/" << seq_state.num_steps
              << (seq_state.is_running ? " [RUNNING]" : " [STOPPED]")
              << std::endl;
  }

  if (state.selected_seq_idx) {
    std::cout << "  Selection: seq=" << *state.selected_seq_idx;
    if (state.selected_step_idx) {
      std::cout << " step=" << *state.selected_step_idx;
    }
    if (state.selected_param_idx) {
      std::cout << " param=" << *state.selected_param_idx;
    }
    std::cout << std::endl;
  }

  last_state_ = state;
}

void Gui::render_sequencer_row(std::size_t seq_idx,
                                const Sequencer_display_state& seq_state) {
  // TODO: Implement GTK grid rendering
  // This would create/update grid cells for this sequencer's steps
}

void Gui::render_selection(const Display_state& state) {
  // TODO: Implement selection highlighting in GTK grid
  // This would highlight the selected cell(s)
}

} // namespace gui

} // namespace Micro_composer
