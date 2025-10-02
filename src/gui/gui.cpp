#include "gui/gui.h"
#include <iostream>
#include <sstream>

namespace Micro_composer {

namespace gui {

// CSS for styling
static const char* CSS_STYLE = R"(
  .playhead {
    background-color: #4CAF50;
    font-weight: bold;
  }
  .selected {
    border: 2px solid #2196F3;
    background-color: #E3F2FD;
  }
  entry {
    min-width: 60px;
    padding: 4px;
  }
)";

Gui::Gui() {
  // GTK will be initialized in init()
}

Gui::~Gui() {
  // GTK cleanup handled by gtk_main_quit if needed
}

void Gui::init(int argc, char** argv) {
  // Initialize GTK
  gtk_init(&argc, &argv);

  // Load CSS
  GtkCssProvider* css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css_provider, CSS_STYLE, -1, nullptr);
  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_USER);

  // Create main window
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window_), "Micro Composer - Step Sequencer");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1024, 768);

  // Connect destroy signal
  g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

  // Create main container
  main_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window_), main_box_);

  // Create scrolled window for grid
  scrolled_window_ = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(main_box_), scrolled_window_, TRUE, TRUE, 0);

  // Create grid
  grid_ = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid_), 2);
  gtk_grid_set_column_spacing(GTK_GRID(grid_), 2);
  gtk_container_add(GTK_CONTAINER(scrolled_window_), grid_);

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

  // Check if we need to rebuild the entire grid (structure changed)
  bool need_rebuild = false;
  if (state.sequencers.size() != last_state_.sequencers.size()) {
    need_rebuild = true;
  } else {
    for (std::size_t i = 0; i < state.sequencers.size(); ++i) {
      if (state.sequencers[i].num_steps != last_state_.sequencers[i].num_steps ||
          state.sequencers[i].num_params !=
              last_state_.sequencers[i].num_params ||
          state.sequencers[i].is_expanded !=
              last_state_.sequencers[i].is_expanded) {
        need_rebuild = true;
        break;
      }
    }
  }

  if (need_rebuild) {
    rebuild_grid(state);
  } else {
    // Just update values and highlighting
    update_cell_values(state);
    update_playhead_highlighting(state);
    update_selection_highlighting(state);
    update_play_icons(state);
  }

  // Scroll to selection if it changed
  if (state.selected_seq_idx != last_state_.selected_seq_idx ||
      state.selected_step_idx != last_state_.selected_step_idx ||
      state.selected_param_idx != last_state_.selected_param_idx) {
    scroll_to_selection(state);
  }

  last_state_ = state;
}

void Gui::rebuild_grid(const Display_state& state) {
  std::cout << "[DEBUG] Rebuilding entire grid..." << std::endl;

  // Clear existing grid contents
  gtk_container_foreach(GTK_CONTAINER(grid_),
                       [](GtkWidget* widget, gpointer) {
                         gtk_widget_destroy(widget);
                       },
                       nullptr);
  cell_widgets_.clear();
  seq_headers_.clear();

  // Calculate maximum number of steps for grid width
  std::size_t max_steps = 0;
  for (const auto& seq : state.sequencers) {
    if (seq.num_steps > max_steps) {
      max_steps = seq.num_steps;
    }
  }

  int current_row = 0;

  // Build each sequence
  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];

    // Create sequence header row
    create_sequence_header(seq_idx, seq, current_row);
    current_row++;

    // If expanded, create parameter rows
    if (seq.is_expanded) {
      for (std::size_t param_idx = 0; param_idx < seq.num_params; ++param_idx) {
        create_parameter_row(seq_idx, param_idx, seq, current_row);
        current_row++;
      }
    }
  }

  gtk_widget_show_all(grid_);
}

void Gui::create_sequence_header(std::size_t seq_idx,
                                  const Sequencer_display_state& seq_state,
                                  int row) {
  // Expand/collapse button (column 0)
  GtkWidget* expand_btn = gtk_button_new_with_label(seq_state.is_expanded ? "▼" : "▶");
  gtk_widget_set_size_request(expand_btn, 30, 30);
  g_object_set_data(G_OBJECT(expand_btn), "seq_idx",
                   GSIZE_TO_POINTER(seq_idx));
  g_signal_connect(expand_btn, "clicked", G_CALLBACK(on_expand_clicked), this);
  gtk_grid_attach(GTK_GRID(grid_), expand_btn, 0, row, 1, 1);

  // Play/pause/stop icon (column 1)
  const char* icon_text = "⏹"; // Stop
  if (seq_state.is_running) {
    icon_text = (seq_state.current_step_idx > 0) ? "▶" : "▶";
  }
  GtkWidget* play_icon = gtk_label_new(icon_text);
  gtk_widget_set_size_request(play_icon, 30, 30);
  gtk_grid_attach(GTK_GRID(grid_), play_icon, 1, row, 1, 1);

  // Sequence name (column 2)
  std::ostringstream name_ss;
  name_ss << "Seq " << seq_idx;
  GtkWidget* name_label = gtk_label_new(name_ss.str().c_str());
  gtk_widget_set_halign(name_label, GTK_ALIGN_START);
  gtk_widget_set_size_request(name_label, 80, 30);
  gtk_grid_attach(GTK_GRID(grid_), name_label, 2, row, 1, 1);

  // Store widgets for updates
  seq_headers_[seq_idx] = {expand_btn, play_icon, name_label};
}

void Gui::create_parameter_row(std::size_t seq_idx, std::size_t param_idx,
                                const Sequencer_display_state& seq_state,
                                int row) {
  // Parameter label (columns 0-2, merged)
  std::ostringstream param_label_ss;
  param_label_ss << "  Param " << param_idx;
  GtkWidget* param_label = gtk_label_new(param_label_ss.str().c_str());
  gtk_widget_set_halign(param_label, GTK_ALIGN_START);
  gtk_widget_set_size_request(param_label, 140, 30);
  gtk_grid_attach(GTK_GRID(grid_), param_label, 0, row, 3, 1);

  // Create cells for each step
  for (std::size_t step_idx = 0; step_idx < seq_state.num_steps; ++step_idx) {
    // Get parameter value
    std::string value = "";
    if (step_idx < seq_state.steps.size() &&
        param_idx < seq_state.steps[step_idx].param_values.size()) {
      value = seq_state.steps[step_idx].param_values[param_idx];
    }

    // Create entry widget
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), value.c_str());
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 8);
    gtk_widget_set_size_request(entry, 70, 30);

    // Store cell info for event handling
    CellWidget cell_widget{entry, seq_idx, step_idx, param_idx};
    std::string key = make_cell_key(seq_idx, step_idx, param_idx);
    cell_widgets_[key] = cell_widget;

    // Attach to user data for callback
    g_object_set_data(G_OBJECT(entry), "cell_key",
                     g_strdup(key.c_str()));

    // Connect edit signal
    g_signal_connect(entry, "activate", G_CALLBACK(on_cell_edited), this);

    // Add to grid (column offset by 3 for fixed columns)
    gtk_grid_attach(GTK_GRID(grid_), entry, step_idx + 3, row, 1, 1);
  }
}

void Gui::update_cell_values(const Display_state& state) {
  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];

    for (std::size_t step_idx = 0; step_idx < seq.steps.size(); ++step_idx) {
      const auto& step = seq.steps[step_idx];

      for (std::size_t param_idx = 0; param_idx < step.param_values.size();
           ++param_idx) {
        std::string key = make_cell_key(seq_idx, step_idx, param_idx);
        auto it = cell_widgets_.find(key);
        if (it != cell_widgets_.end()) {
          const char* current_text =
              gtk_entry_get_text(GTK_ENTRY(it->second.entry));
          if (std::string(current_text) != step.param_values[param_idx]) {
            gtk_entry_set_text(GTK_ENTRY(it->second.entry),
                              step.param_values[param_idx].c_str());
          }
        }
      }
    }
  }
}

void Gui::update_playhead_highlighting(const Display_state& state) {
  // Clear old playhead highlighting
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context =
        gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "playhead");
  }

  // Add new playhead highlighting
  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];
    std::size_t current_step = seq.current_step_idx;

    // Highlight all parameters for the current step of this sequence
    for (std::size_t param_idx = 0; param_idx < seq.num_params; ++param_idx) {
      std::string key = make_cell_key(seq_idx, current_step, param_idx);
      auto it = cell_widgets_.find(key);
      if (it != cell_widgets_.end()) {
        GtkStyleContext* context =
            gtk_widget_get_style_context(it->second.entry);
        gtk_style_context_add_class(context, "playhead");
      }
    }
  }
}

void Gui::update_selection_highlighting(const Display_state& state) {
  // Clear old selection highlighting
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context =
        gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "selected");
  }

  // Add new selection highlighting
  if (state.selected_seq_idx && state.selected_step_idx &&
      state.selected_param_idx) {
    std::string key = make_cell_key(*state.selected_seq_idx,
                                   *state.selected_step_idx,
                                   *state.selected_param_idx);
    auto it = cell_widgets_.find(key);
    if (it != cell_widgets_.end()) {
      GtkStyleContext* context =
          gtk_widget_get_style_context(it->second.entry);
      gtk_style_context_add_class(context, "selected");
    }
  }
}

void Gui::update_play_icons(const Display_state& state) {
  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];
    auto it = seq_headers_.find(seq_idx);
    if (it != seq_headers_.end()) {
      const char* icon_text = "⏹"; // Stop
      if (seq.is_running) {
        icon_text = "▶"; // Play
      }
      gtk_label_set_text(GTK_LABEL(it->second.play_icon), icon_text);
    }
  }
}

std::string Gui::make_cell_key(std::size_t seq, std::size_t step,
                                std::size_t param) const {
  std::ostringstream ss;
  ss << seq << "_" << step << "_" << param;
  return ss.str();
}

void Gui::clear_highlighting() {
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context =
        gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "playhead");
    gtk_style_context_remove_class(context, "selected");
  }
}

void Gui::scroll_to_selection(const Display_state& state) {
  if (!state.selected_seq_idx || !state.selected_step_idx ||
      !state.selected_param_idx) {
    return;
  }

  std::string key = make_cell_key(*state.selected_seq_idx,
                                 *state.selected_step_idx,
                                 *state.selected_param_idx);
  auto it = cell_widgets_.find(key);
  if (it != cell_widgets_.end()) {
    // Scroll to make the selected cell visible
    GtkAdjustment* hadj =
        gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window_));
    GtkAdjustment* vadj =
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window_));

    // Get widget allocation
    GtkAllocation alloc;
    gtk_widget_get_allocation(it->second.entry, &alloc);

    // Scroll horizontally if needed
    gdouble h_value = gtk_adjustment_get_value(hadj);
    gdouble h_page = gtk_adjustment_get_page_size(hadj);
    if (alloc.x < h_value) {
      gtk_adjustment_set_value(hadj, alloc.x);
    } else if (alloc.x + alloc.width > h_value + h_page) {
      gtk_adjustment_set_value(hadj, alloc.x + alloc.width - h_page);
    }

    // Scroll vertically if needed
    gdouble v_value = gtk_adjustment_get_value(vadj);
    gdouble v_page = gtk_adjustment_get_page_size(vadj);
    if (alloc.y < v_value) {
      gtk_adjustment_set_value(vadj, alloc.y);
    } else if (alloc.y + alloc.height > v_value + v_page) {
      gtk_adjustment_set_value(vadj, alloc.y + alloc.height - v_page);
    }
  }
}

// Static event handlers
void Gui::on_cell_edited(GtkEntry* entry, gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);
  const char* key = static_cast<const char*>(
      g_object_get_data(G_OBJECT(entry), "cell_key"));

  if (key) {
    auto it = gui->cell_widgets_.find(key);
    if (it != gui->cell_widgets_.end()) {
      const char* new_value = gtk_entry_get_text(entry);
      std::cout << "[INFO] Cell edited: seq=" << it->second.seq_idx
                << " step=" << it->second.step_idx
                << " param=" << it->second.param_idx << " value=" << new_value
                << std::endl;

      // TODO: Call controller to update the actual parameter value
      // This will require passing the controller reference to the GUI
    }
  }
}

void Gui::on_expand_clicked(GtkButton* button, gpointer user_data) {
  std::size_t seq_idx =
      GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(button), "seq_idx"));
  std::cout << "[INFO] Expand/collapse clicked for seq " << seq_idx
            << std::endl;

  // TODO: Toggle expansion state in controller and trigger re-render
  // For now, this would require the controller to track expansion state
}

} // namespace gui

} // namespace Micro_composer
