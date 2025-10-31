#include "gui/event_parameter_traits.h"
#include "gui/gui.h"
#include "utility/debug.h"
#include <cstring>
#include <fstream>
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace Micro_composer {

namespace gui {

// Constructor
template <sequencable::Mut_seq_event Event_t>
Gui<Event_t>::Gui(Controller& controller, unsigned int fps)
    : controller_{controller}, fps_{fps},
      frame_duration_{1.0 / static_cast<double>(fps)} {
  // Initialize state
  state_.controller_state = controller_.get_state();
  state_.sequencer_gui_states.resize(state_.controller_state.sizes.size());

  // Select first sequencer and first event by default
  if (!state_.controller_state.sizes.empty() &&
      state_.controller_state.sizes[0] > 0) {
    controller_.select(0, 0);
    state_.controller_state = controller_.get_state();
  }

  // Initialize keyboard mappings
  // Normal mode mappings
  normal_mode_actions_[GDK_KEY_h] = [this]() { gui_select_prev_pos(); };

  normal_mode_actions_[GDK_KEY_j] = [this]() { gui_select_next_param(); };

  normal_mode_actions_[GDK_KEY_k] = [this]() { gui_select_prev_param(); };

  normal_mode_actions_[GDK_KEY_l] = [this]() { gui_select_next_pos(); };

  normal_mode_actions_[GDK_KEY_space] = [this]() {
    auto sel_seq = controller_.selected_seq();
    if (sel_seq) {
      gui_toggle_play(*sel_seq);
    }
  };

  normal_mode_actions_[GDK_KEY_t] = [this]() { gui_toggle_sequencer(); };

  normal_mode_actions_[GDK_KEY_i] = [this]() {
    state_.mode = Mode::Edit;
    state_.state_dirty = true;
    update_window_title();
    focus_selected_cell();
    // Note: Keep multi-selection active when entering edit mode
  };

  normal_mode_actions_[GDK_KEY_Return] = [this]() {
    auto sel_seq = controller_.selected_seq();
    auto sel_evt = controller_.selected_event();
    if (sel_seq && sel_evt) {
      controller_.toggle(*sel_seq, *sel_evt);
      // Update the enabled cell (param index 0)
      update_cell_value(*sel_seq, *sel_evt, 0);
      state_.state_dirty = true;
    }
  };

  // Keys 1-8 select events at indexes 0-7
  for (guint key = GDK_KEY_1; key <= GDK_KEY_8; ++key) {
    normal_mode_actions_[key] = [this, key]() {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        Event_idx event_idx = key - GDK_KEY_1; // Convert key to index (1->0, 2->1, etc.)
        auto& state = state_.controller_state;
        // Bounds check
        if (*sel_seq < state.sizes.size() && event_idx < state.sizes[*sel_seq]) {
          controller_.select(*sel_seq, event_idx);
          state_.state_dirty = true;
        }
      }
    };
  }

  // UP key increments selected cell value
  normal_mode_actions_[GDK_KEY_Up] = [this]() {
    auto sel_seq = controller_.selected_seq();
    auto sel_evt = controller_.selected_event();
    if (sel_seq && sel_evt) {
      size_t param_idx = state_.selected_param_idx;
      increment_cell_value(*sel_seq, *sel_evt, param_idx, true);
    }
  };

  // DOWN key decrements selected cell value
  normal_mode_actions_[GDK_KEY_Down] = [this]() {
    auto sel_seq = controller_.selected_seq();
    auto sel_evt = controller_.selected_event();
    if (sel_seq && sel_evt) {
      size_t param_idx = state_.selected_param_idx;
      increment_cell_value(*sel_seq, *sel_evt, param_idx, false);
    }
  };

  // Edit mode mappings
  edit_mode_actions_[GDK_KEY_Escape] = [this]() {
    state_.mode = Mode::Normal;
    clear_multi_selection();  // Clear multi-selection when exiting edit mode
    state_.state_dirty = true;
    update_window_title();
  };

  // Load preferences
  load_preferences();

  // Initialize GTK
  gtk_init(nullptr, nullptr);
  init_widgets();
  apply_preferences();  // Apply preferences after widgets are created
  update_window_title();
}

// Destructor
template <sequencable::Mut_seq_event Event_t> Gui<Event_t>::~Gui() {
  // Save current window size to preferences
  if (window_) {
    gtk_window_get_size(GTK_WINDOW(window_),
                       &preferences_.window_width,
                       &preferences_.window_height);
    gtk_widget_destroy(window_);
  }

  // Save preferences to config file
  save_preferences();
}

// Run the GUI
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::run() {
  running_ = true;
  gtk_widget_show_all(window_);

  // Add timeout for event loop ticks
  g_timeout_add(static_cast<guint>(frame_duration_.count() * 1000), on_tick,
                this);

  gtk_main();
}

// Initialize GTK widgets
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::init_widgets() {
  // Create main window
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window_), "MicroComposer");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1000, 700);

  // Connect destroy signal
  g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

  // Connect key press signal
  g_signal_connect(window_, "key-press-event", G_CALLBACK(on_key_press), this);

  // Load CSS
  GtkCssProvider* css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css_provider, "resources/gui_style.css",
                                  nullptr);
  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Create main vertical box
  main_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window_), main_vbox_);

  // Build menu bar
  build_menu_bar();

  // Create error label (initially hidden)
  error_label_ = gtk_label_new("");
  gtk_widget_set_name(error_label_, "error-label");
  gtk_box_pack_start(GTK_BOX(main_vbox_), error_label_, FALSE, FALSE, 0);
  gtk_widget_set_no_show_all(error_label_, TRUE); // Don't show by default

  // Create scrolled window for sequencer widgets
  scrolled_window_ = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(main_vbox_), scrolled_window_, TRUE, TRUE, 0);

  // Create vertical box to hold sequencer widgets
  sequencers_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(scrolled_window_), sequencers_vbox_);

  // Build sequencer widgets
  build_sequencer_widgets();
}

// Build sequencer widgets (one per sequencer)
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::build_sequencer_widgets() {
  using Traits = Event_parameter_traits<Event_t>;
  const auto& state = state_.controller_state;

  // Destroy and remove old widgets from container
  for (auto& widget : sequencer_widgets_) {
    if (widget.frame) {
      // Removing the frame from the container will destroy all child widgets
      gtk_container_remove(GTK_CONTAINER(sequencers_vbox_), widget.frame);
      // The g_object_set_data_full cleanup will be called automatically
    }
  }
  sequencer_widgets_.clear();

  constexpr size_t num_params = Traits::parameter_count;

  // Create a widget for each sequencer
  for (Seq_idx seq_idx = 0; seq_idx < state.sizes.size(); ++seq_idx) {
    Sequencer_widget widget;
    const size_t num_events = state.sizes[seq_idx];

    // Create frame
    widget.frame = gtk_frame_new(nullptr);
    gtk_widget_set_name(widget.frame, "sequencer-frame");
    gtk_frame_set_shadow_type(GTK_FRAME(widget.frame), GTK_SHADOW_ETCHED_IN);

    // Create vertical box
    widget.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(widget.frame), widget.vbox);

    // Create header label with sequencer status
    std::string header_text = "Sequencer " + std::to_string(seq_idx);
    header_text += " [" + std::to_string(num_events) + " steps]";
    if (seq_idx < state.scheduling.size() && state.scheduling[seq_idx]) {
      header_text += " ▶";
    } else {
      header_text += " ⏸";
    }
    if (controller_.is_sequencer_toggled(seq_idx)) {
      header_text += " 🔇";
    }
    widget.header_label = gtk_label_new(header_text.c_str());
    gtk_widget_set_name(widget.header_label, "sequencer-header");
    gtk_box_pack_start(GTK_BOX(widget.vbox), widget.header_label, FALSE, FALSE,
                       2);

    // Create column headers with event indices
    widget.column_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_name(widget.column_header, "column-headers");

    // Add spacer for parameter name column (80px to match row labels)
    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_size_request(spacer, 80, -1);
    gtk_box_pack_start(GTK_BOX(widget.column_header), spacer, FALSE, FALSE, 0);

    // Add column headers for each event
    for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
      std::string col_label = std::to_string(evt_idx);
      GtkWidget* col_header_label = gtk_label_new(col_label.c_str());
      gtk_widget_set_size_request(col_header_label, 70,
                                  -1); // Match entry width
      gtk_box_pack_start(GTK_BOX(widget.column_header), col_header_label, FALSE,
                         FALSE, 2);
    }

    gtk_box_pack_start(GTK_BOX(widget.vbox), widget.column_header, FALSE, FALSE,
                       2);

    // Create grid for parameters
    widget.grid = gtk_grid_new();
    gtk_widget_set_name(widget.grid, "sequencer-grid");
    gtk_grid_set_row_spacing(GTK_GRID(widget.grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(widget.grid), 2);
    gtk_box_pack_start(GTK_BOX(widget.vbox), widget.grid, TRUE, TRUE, 0);

    // Build parameter rows
    widget.cells.resize(num_params);
    widget.row_labels.resize(num_params);

    for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
      // Create row label
      GtkWidget* label =
          gtk_label_new(Traits::get_parameter_name(param_idx).c_str());
      gtk_widget_set_size_request(label, 80, -1);
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_grid_attach(GTK_GRID(widget.grid), label, 0, param_idx, 1, 1);
      widget.row_labels[param_idx] = label;

      // Create entry widgets for each event
      for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);
        gtk_widget_set_size_request(entry, 70, -1);

        // Set initial value
        if (seq_idx < state.events.size() &&
            evt_idx < state.events[seq_idx].size()) {
          const auto& event = state.events[seq_idx][evt_idx];
          std::string value_str = Traits::get_parameter_value(event, param_idx);
          gtk_entry_set_text(GTK_ENTRY(entry), value_str.c_str());
        }

        // Allocate user data for callbacks
        auto* user_data =
            new Entry_user_data{this, seq_idx, evt_idx, param_idx};

        // Connect signals
        g_signal_connect(entry, "focus-out-event",
                         G_CALLBACK(on_entry_focus_out), user_data);
        g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate),
                         user_data);
        g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed),
                         user_data);

        // Store cleanup data
        g_object_set_data_full(G_OBJECT(entry), "user-data", user_data, g_free);

        gtk_grid_attach(GTK_GRID(widget.grid), entry, evt_idx + 1, param_idx, 1,
                        1);
        widget.cells[param_idx].push_back(entry);
      }
    }

    // Add to container
    gtk_box_pack_start(GTK_BOX(sequencers_vbox_), widget.frame, FALSE, FALSE,
                       5);
    sequencer_widgets_.push_back(widget);
  }

  // Show all newly created widgets
  gtk_widget_show_all(sequencers_vbox_);
}

// Rebuild a single sequencer widget efficiently
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::rebuild_sequencer_widget(Seq_idx seq_idx) {
  using Traits = Event_parameter_traits<Event_t>;

  // Bounds check
  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  // Get fresh state
  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  if (seq_idx >= state.sizes.size()) {
    return;
  }

  // Remove old widget from container
  auto& old_widget = sequencer_widgets_[seq_idx];
  if (old_widget.frame) {
    gtk_container_remove(GTK_CONTAINER(sequencers_vbox_), old_widget.frame);
  }

  // Build new widget
  Sequencer_widget widget;
  const size_t num_events = state.sizes[seq_idx];
  constexpr size_t num_params = Traits::parameter_count;

  // Create frame
  widget.frame = gtk_frame_new(nullptr);
  gtk_widget_set_name(widget.frame, "sequencer-frame");
  gtk_frame_set_shadow_type(GTK_FRAME(widget.frame), GTK_SHADOW_ETCHED_IN);

  // Create vertical box
  widget.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add(GTK_CONTAINER(widget.frame), widget.vbox);

  // Create header label with sequencer status
  std::string header_text = "Sequencer " + std::to_string(seq_idx);
  header_text += " [" + std::to_string(num_events) + " steps]";
  if (seq_idx < state.scheduling.size() && state.scheduling[seq_idx]) {
    header_text += " ▶";
  } else {
    header_text += " ⏸";
  }
  if (controller_.is_sequencer_toggled(seq_idx)) {
    header_text += " 🔇";
  }
  widget.header_label = gtk_label_new(header_text.c_str());
  gtk_widget_set_name(widget.header_label, "sequencer-header");
  gtk_box_pack_start(GTK_BOX(widget.vbox), widget.header_label, FALSE, FALSE, 2);

  // Create column headers with event indices
  widget.column_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_name(widget.column_header, "column-headers");

  // Add spacer for parameter name column
  GtkWidget* spacer = gtk_label_new("");
  gtk_widget_set_size_request(spacer, 80, -1);
  gtk_box_pack_start(GTK_BOX(widget.column_header), spacer, FALSE, FALSE, 0);

  // Add column headers for each event
  for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
    std::string col_label = std::to_string(evt_idx);
    GtkWidget* col_header_label = gtk_label_new(col_label.c_str());
    gtk_widget_set_size_request(col_header_label, 70, -1);
    gtk_box_pack_start(GTK_BOX(widget.column_header), col_header_label, FALSE, FALSE, 2);
  }

  gtk_box_pack_start(GTK_BOX(widget.vbox), widget.column_header, FALSE, FALSE, 2);

  // Create grid for parameters
  widget.grid = gtk_grid_new();
  gtk_widget_set_name(widget.grid, "sequencer-grid");
  gtk_grid_set_row_spacing(GTK_GRID(widget.grid), 2);
  gtk_grid_set_column_spacing(GTK_GRID(widget.grid), 2);
  gtk_box_pack_start(GTK_BOX(widget.vbox), widget.grid, TRUE, TRUE, 0);

  // Build parameter rows
  widget.cells.resize(num_params);
  widget.row_labels.resize(num_params);

  for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
    // Create row label
    GtkWidget* label = gtk_label_new(Traits::get_parameter_name(param_idx).c_str());
    gtk_widget_set_size_request(label, 80, -1);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(widget.grid), label, 0, param_idx, 1, 1);
    widget.row_labels[param_idx] = label;

    // Create entry widgets for each event
    for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
      GtkWidget* entry = gtk_entry_new();
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);
      gtk_widget_set_size_request(entry, 70, -1);

      // Set initial value
      if (seq_idx < state.events.size() &&
          evt_idx < state.events[seq_idx].size()) {
        const auto& event = state.events[seq_idx][evt_idx];
        std::string value_str = Traits::get_parameter_value(event, param_idx);
        gtk_entry_set_text(GTK_ENTRY(entry), value_str.c_str());
      }

      // Allocate user data for callbacks
      auto* user_data = new Entry_user_data{this, seq_idx, evt_idx, param_idx};

      // Connect signals
      g_signal_connect(entry, "focus-out-event",
                       G_CALLBACK(on_entry_focus_out), user_data);
      g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate),
                       user_data);
      g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed),
                       user_data);

      // Store cleanup data
      g_object_set_data_full(G_OBJECT(entry), "user-data", user_data, g_free);

      gtk_grid_attach(GTK_GRID(widget.grid), entry, evt_idx + 1, param_idx, 1, 1);
      widget.cells[param_idx].push_back(entry);
    }
  }

  // Insert at correct position in container
  gtk_box_pack_start(GTK_BOX(sequencers_vbox_), widget.frame, FALSE, FALSE, 5);
  gtk_box_reorder_child(GTK_BOX(sequencers_vbox_), widget.frame, seq_idx);

  // Replace in our vector
  sequencer_widgets_[seq_idx] = widget;

  // Show the new widget
  gtk_widget_show_all(widget.frame);
}

// Parse and apply edit to event parameter
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::parse_and_apply_edit(Seq_idx seq_idx, Event_idx event_idx,
                                        size_t param_idx,
                                        const std::string& value_str) {
  using Traits = Event_parameter_traits<Event_t>;

  try {
    // Apply mutation via controller
    controller_.mutate(seq_idx, event_idx,
                       [param_idx, &value_str](Event_t&& evt) {
                         Traits::set_parameter_value(evt, param_idx, value_str);
                         return std::move(evt);
                       });
    return true;
  } catch (const std::exception& e) {
    show_error(e.what());
    return false;
  }
}

// Show error message (auto-clears after 5 seconds)
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::show_error(const std::string& message) {
  // Cancel existing timeout if any
  if (error_timeout_id_ != 0) {
    g_source_remove(error_timeout_id_);
    error_timeout_id_ = 0;
  }

  // Set error message and show
  gtk_label_set_text(GTK_LABEL(error_label_), message.c_str());
  gtk_widget_show(error_label_);

  // Set timeout to clear after 5 seconds
  error_timeout_id_ = g_timeout_add(5000, clear_error_timeout, this);
}

// Timeout callback to clear error message
template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::clear_error_timeout(gpointer user_data) {
  auto* gui = static_cast<Gui*>(user_data);
  gtk_widget_hide(gui->error_label_);
  gui->error_timeout_id_ = 0;
  return FALSE; // Don't repeat
}

// Entry focus-out callback - apply edit when focus leaves
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_focus_out(GtkWidget* widget, GdkEventFocus* event,
                                      gpointer user_data) {
  (void)event; // Unused
  auto* data = static_cast<Entry_user_data*>(user_data);
  auto* gui = data->gui;

  // Get the entered text
  const char* text = gtk_entry_get_text(GTK_ENTRY(widget));
  std::string value_str(text);

  // Check if multi-selection is active
  if (!gui->state_.selected_event_range.empty()) {
    // Apply to all selected cells
    bool all_success = true;
    for (Event_idx evt_idx : gui->state_.selected_event_range) {
      bool success = gui->parse_and_apply_edit(data->seq_idx, evt_idx,
                                                data->param_idx, value_str);
      all_success = all_success && success;
    }

    if (all_success) {
      // Refresh all successfully edited cells to show new values
      for (Event_idx evt_idx : gui->state_.selected_event_range) {
        gui->update_cell_value(data->seq_idx, evt_idx, data->param_idx);
      }
    } else {
      // Restore all cells to original values on failure
      using Traits = Event_parameter_traits<Event_t>;
      for (Event_idx evt_idx : gui->state_.selected_event_range) {
        gui->update_cell_value(data->seq_idx, evt_idx, data->param_idx);
      }
    }

    // Clear multi-selection after apply
    gui->clear_multi_selection();
    gui->state_.state_dirty = true;
  } else {
    // Single cell edit
    bool success = gui->parse_and_apply_edit(data->seq_idx, data->event_idx,
                                             data->param_idx, value_str);

    if (success) {
      // Update state and mark dirty
      gui->state_.state_dirty = true;
    } else {
      // Restore original value on failure
      using Traits = Event_parameter_traits<Event_t>;
      auto& state = gui->state_.controller_state;
      if (data->seq_idx < state.events.size() &&
          data->event_idx < state.events[data->seq_idx].size()) {
        const auto& event = state.events[data->seq_idx][data->event_idx];
        std::string original_value =
            Traits::get_parameter_value(event, data->param_idx);
        gtk_entry_set_text(GTK_ENTRY(widget), original_value.c_str());
      }
    }
  }
}

// Entry activate callback - apply edit when Enter is pressed
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_activate(GtkEntry* entry, gpointer user_data) {
  auto* data = static_cast<Entry_user_data*>(user_data);
  auto* gui = data->gui;

  // Get the entered text
  const char* text = gtk_entry_get_text(entry);
  std::string value_str(text);

  // Check if multi-selection is active
  if (!gui->state_.selected_event_range.empty()) {
    // Apply to all selected cells
    bool all_success = true;
    for (Event_idx evt_idx : gui->state_.selected_event_range) {
      bool success = gui->parse_and_apply_edit(data->seq_idx, evt_idx,
                                                data->param_idx, value_str);
      all_success = all_success && success;
    }

    if (all_success) {
      // Refresh all successfully edited cells to show new values
      for (Event_idx evt_idx : gui->state_.selected_event_range) {
        gui->update_cell_value(data->seq_idx, evt_idx, data->param_idx);
      }
      // Success - remove focus from entry to exit edit mode visually
      gtk_widget_grab_focus(gui->window_);
    } else {
      // Restore all cells to original values on failure
      using Traits = Event_parameter_traits<Event_t>;
      for (Event_idx evt_idx : gui->state_.selected_event_range) {
        gui->update_cell_value(data->seq_idx, evt_idx, data->param_idx);
      }
    }

    // Clear multi-selection after apply
    gui->clear_multi_selection();
    gui->state_.state_dirty = true;
  } else {
    // Single cell edit
    bool success = gui->parse_and_apply_edit(data->seq_idx, data->event_idx,
                                             data->param_idx, value_str);

    if (success) {
      // Update state and mark dirty
      gui->state_.state_dirty = true;

      // Remove focus from entry to exit edit mode visually
      gtk_widget_grab_focus(gui->window_);
    } else {
      // Restore original value on failure
      using Traits = Event_parameter_traits<Event_t>;
      auto& state = gui->state_.controller_state;
      if (data->seq_idx < state.events.size() &&
          data->event_idx < state.events[data->seq_idx].size()) {
        const auto& event = state.events[data->seq_idx][data->event_idx];
        std::string original_value =
            Traits::get_parameter_value(event, data->param_idx);
        gtk_entry_set_text(entry, original_value.c_str());
      }
    }
  }
}

// Entry changed callback - propagate text to all selected cells
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_changed(GtkEntry* entry, gpointer user_data) {
  auto* data = static_cast<Entry_user_data*>(user_data);
  auto* gui = data->gui;

  // Prevent recursion
  if (gui->state_.in_text_update) {
    return;
  }

  // Only propagate if multi-selection is active
  if (gui->state_.selected_event_range.empty()) {
    return;
  }

  // Only propagate from the currently selected cell
  auto sel_seq = gui->controller_.selected_seq();
  auto sel_evt = gui->controller_.selected_event();
  if (!sel_seq || !sel_evt || data->event_idx != *sel_evt) {
    return;
  }

  // Get current text
  const char* text = gtk_entry_get_text(entry);

  // Block recursion
  gui->state_.in_text_update = true;

  // Update all other selected cells in the same row
  if (*sel_seq < gui->sequencer_widgets_.size()) {
    auto& widget = gui->sequencer_widgets_[*sel_seq];
    size_t param_idx = gui->state_.selected_param_idx;

    for (Event_idx evt_idx : gui->state_.selected_event_range) {
      if (evt_idx != data->event_idx &&
          param_idx < widget.cells.size() &&
          evt_idx < widget.cells[param_idx].size()) {
        GtkWidget* target_cell = widget.cells[param_idx][evt_idx];
        gtk_entry_set_text(GTK_ENTRY(target_cell), text);
      }
    }
  }

  gui->state_.in_text_update = false;
}

// GTK key press callback
template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_key_press(GtkWidget* widget, GdkEventKey* event,
                                    gpointer user_data) {
  auto* gui = static_cast<Gui*>(user_data);

  // Check for F1 (help dialog) in any mode
  if (event->keyval == GDK_KEY_F1) {
    gui->show_help_dialog();
    return TRUE;
  }

  // Check for Ctrl+A (add event) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_a) {
    gui->gui_add_event();
    return TRUE;
  }

  // Check for Ctrl+D (remove event) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d) {
    gui->gui_remove_event();
    return TRUE;
  }

  // Check for Ctrl+C (clear sequence) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_c) {
    gui->gui_clear_sequence();
    return TRUE;
  }

  // Check for Ctrl+J (next sequencer) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_j) {
    gui->gui_select_next_seq();
    return TRUE;
  }

  // Check for Ctrl+K (previous sequencer) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_k) {
    gui->gui_select_prev_seq();
    return TRUE;
  }

  // Check for Ctrl+R (select all in row) in any mode
  if ((event->state & GDK_CONTROL_MASK) &&
      (event->keyval == GDK_KEY_r || event->keyval == GDK_KEY_R)) {
    gui->gui_select_all_in_row();
    return TRUE;
  }

  // Check for Ctrl+Space (start/stop all sequencers) in any mode
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_space) {
    auto time = Controller::Clock::now() + std::chrono::milliseconds(50);
    if (gui->controller_.any_scheduling()) {
      gui->controller_.pause(time);
    } else {
      gui->controller_.start(time, true); // true for repeat
    }
    gui->state_.state_dirty = true;
    return TRUE;
  }

  // Check for Ctrl+0 (reset selected sequencer) or Ctrl+) (reset all sequencers)
  if ((event->state & GDK_CONTROL_MASK) &&
      (event->keyval == GDK_KEY_0 || event->keyval == GDK_KEY_parenright)) {
    auto time = Controller::Clock::now() + std::chrono::milliseconds(50);

    if (event->state & GDK_SHIFT_MASK || event->keyval == GDK_KEY_parenright) {
      // Ctrl+Shift+0 or Ctrl+) - Reset all sequencers
      gui->controller_.stop(time, 0);

      // Update all playhead visuals to position 0
      for (Seq_idx seq_idx = 0; seq_idx < gui->state_.sequencer_gui_states.size(); ++seq_idx) {
        auto& gui_state = gui->state_.sequencer_gui_states[seq_idx];
        gui->update_playhead_visual(seq_idx, gui_state.last_rendered_playhead, 0);
        gui_state.last_rendered_playhead = 0;
        gui_state.playhead_visible = true;
      }
    } else {
      // Ctrl+0 - Reset selected sequencer
      auto sel_seq = gui->controller_.selected_seq();
      if (sel_seq) {
        gui->gui_stop(*sel_seq);
      }
    }

    gui->state_.state_dirty = true;
    return TRUE;
  }

  // Check for Shift+H (extend selection left) in normal mode
  if ((event->state & GDK_SHIFT_MASK) &&
      (event->keyval == GDK_KEY_h || event->keyval == GDK_KEY_H) &&
      gui->state_.mode == Mode::Normal) {
    gui->gui_extend_selection_left();
    return TRUE;
  }

  // Check for Shift+L (extend selection right) in normal mode
  if ((event->state & GDK_SHIFT_MASK) &&
      (event->keyval == GDK_KEY_l || event->keyval == GDK_KEY_L) &&
      gui->state_.mode == Mode::Normal) {
    gui->gui_extend_selection_right();
    return TRUE;
  }

  // Check for Shift+8 (asterisk) to enter tempo multiply mode
  if ((event->state & GDK_SHIFT_MASK) && event->keyval == GDK_KEY_8 &&
      gui->state_.mode == Mode::Normal) {
    gui->gui_enter_tempo_multiply_mode();
    return TRUE;
  }

  // Handle tempo multiply mode input
  if (gui->state_.in_tempo_multiply_mode) {
    if (event->keyval == GDK_KEY_Return) {
      // Apply the multiplication
      gui->gui_apply_tempo_multiply();
      return TRUE;
    } else if (event->keyval == GDK_KEY_Escape) {
      // Cancel tempo multiply mode
      gui->gui_cancel_tempo_multiply();
      return TRUE;
    } else if (event->keyval == GDK_KEY_BackSpace) {
      // Remove last character
      if (!gui->state_.tempo_input_buffer.empty()) {
        gui->state_.tempo_input_buffer.pop_back();
        gui->update_window_title();
      }
      return TRUE;
    } else if (event->keyval >= GDK_KEY_0 && event->keyval <= GDK_KEY_9) {
      // Add digit to buffer
      gui->state_.tempo_input_buffer += static_cast<char>('0' + (event->keyval - GDK_KEY_0));
      gui->update_window_title();
      return TRUE;
    } else if (event->keyval == GDK_KEY_period && !(event->state & GDK_SHIFT_MASK)) {
      // Add decimal point to buffer (but not if Shift is held, which is '>')
      gui->state_.tempo_input_buffer += '.';
      gui->update_window_title();
      return TRUE;
    }
    // Ignore other keys in tempo multiply mode
    return TRUE;
  }

  // Check for Shift+period ('>') key (increment tempo) - with Ctrl for all sequencers
  if ((event->state & GDK_SHIFT_MASK) && event->keyval == GDK_KEY_period) {
    constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
    if (event->state & GDK_CONTROL_MASK) {
      gui->gui_adjust_durations_all(TEMPO_INCREMENT_DELTA, true);
    } else {
      auto sel_seq = gui->controller_.selected_seq();
      if (sel_seq) {
        gui->gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, true);
      }
    }
    return TRUE;
  }

  // Check for Shift+comma ('<') key (decrement tempo) - with Ctrl for all sequencers
  if ((event->state & GDK_SHIFT_MASK) && event->keyval == GDK_KEY_comma) {
    constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
    if (event->state & GDK_CONTROL_MASK) {
      gui->gui_adjust_durations_all(TEMPO_INCREMENT_DELTA, false);
    } else {
      auto sel_seq = gui->controller_.selected_seq();
      if (sel_seq) {
        gui->gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, false);
      }
    }
    return TRUE;
  }

  if (gui->state_.mode == Mode::Normal) {
    // In normal mode, we handle all keys and consume them
    gui->handle_normal_mode_key(event->keyval);
    return TRUE; // Consume event, don't propagate to entry widgets
  } else {
    // In edit mode, check if it's the escape key
    if (event->keyval == GDK_KEY_Escape) {
      gui->handle_edit_mode_key(event->keyval);
      return TRUE; // Consume escape key
    }
    // Allow other keys to propagate to entry widgets for editing
    return FALSE;
  }
}

// Handle normal mode keyboard input
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::handle_normal_mode_key(guint keyval) {
  auto it = normal_mode_actions_.find(keyval);
  if (it != normal_mode_actions_.end()) {
    it->second(); // Execute the mapped action
  }
}

// Handle edit mode keyboard input
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::handle_edit_mode_key(guint keyval) {
  auto it = edit_mode_actions_.find(keyval);
  if (it != edit_mode_actions_.end()) {
    it->second(); // Execute the mapped action
  }
}

// Event loop tick callback
template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_tick(gpointer user_data) {
  auto* gui = static_cast<Gui*>(user_data);

  if (!gui->running_) {
    return FALSE; // Stop the timeout
  }

  // Event loop steps:
  // 1. Process input events (handled by GTK callbacks)
  // 2. Update playheads for running sequencers
  gui->update_playheads();
  // 3. Render if state is dirty
  if (gui->state_.state_dirty) {
    gui->render();
    gui->state_.state_dirty = false;
  }

  return TRUE; // Continue the timeout
}

// Update playhead positions for running sequencers only
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_playheads() {
  // Get fresh controller state (just dynamic fields)
  auto current_state = controller_.get_state();

  // Check each running sequencer for playhead changes
  for (Seq_idx seq_idx = 0; seq_idx < current_state.positions.size();
       ++seq_idx) {
    if (seq_idx >= state_.sequencer_gui_states.size()) {
      continue;
    }

    // Skip non-running sequencers - their playheads stay where they are
    if (!current_state.scheduling[seq_idx]) {
      continue;
    }

    auto& gui_state = state_.sequencer_gui_states[seq_idx];
    Event_idx current_pos = current_state.positions[seq_idx];

    // If playhead moved, update visual
    if (!gui_state.playhead_visible ||
        gui_state.last_rendered_playhead != current_pos) {
      update_playhead_visual(seq_idx, gui_state.last_rendered_playhead,
                             current_pos);
      gui_state.last_rendered_playhead = current_pos;
      gui_state.playhead_visible = true;
    }
  }
}

// Update playhead visual using CSS classes
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_playhead_visual(Seq_idx seq_idx, Event_idx old_pos,
                                          Event_idx new_pos) {
  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  const auto& widget = sequencer_widgets_[seq_idx];

  // Update playhead class for all parameter rows
  for (const auto& param_row : widget.cells) {
    // Remove playhead class from old cell
    if (old_pos < param_row.size()) {
      GtkWidget* old_cell = param_row[old_pos];
      GtkStyleContext* context = gtk_widget_get_style_context(old_cell);
      gtk_style_context_remove_class(context, "playhead");
    }

    // Add playhead class to new cell
    if (new_pos < param_row.size()) {
      GtkWidget* new_cell = param_row[new_pos];
      GtkStyleContext* context = gtk_widget_get_style_context(new_cell);
      gtk_style_context_add_class(context, "playhead");
    }
  }
}

// Render the GUI
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::render() {
  // Update controller state
  state_.controller_state = controller_.get_state();

  // Render grid (for now, just rebuild on dirty state)
  render_grid();
}

// Render the grid
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::render_grid() {
  const auto& state = state_.controller_state;

  // Check if selection changed
  bool selection_changed = (state.selected_seq != state_.last_selected_seq) ||
                           (state.selected_event != state_.last_selected_event) ||
                           (state_.selected_param_idx != state_.last_selected_param);

  if (!selection_changed) {
    return; // Nothing to update
  }

  // Remove selection from previous cell
  if (state_.last_selected_seq && state_.last_selected_event && state_.last_selected_param) {
    Seq_idx prev_seq = *state_.last_selected_seq;
    Event_idx prev_evt = *state_.last_selected_event;
    size_t prev_param = *state_.last_selected_param;

    if (prev_seq < sequencer_widgets_.size()) {
      const auto& widget = sequencer_widgets_[prev_seq];
      if (prev_param < widget.cells.size() && prev_evt < widget.cells[prev_param].size()) {
        GtkWidget* cell = widget.cells[prev_param][prev_evt];
        GtkStyleContext* context = gtk_widget_get_style_context(cell);
        gtk_style_context_remove_class(context, "selected");
      }

      // Remove multi-selection classes from previously selected range
      if (!state_.last_selected_event_range.empty()) {
        for (Event_idx evt_idx : state_.last_selected_event_range) {
          if (prev_param < widget.cells.size() && evt_idx < widget.cells[prev_param].size()) {
            GtkWidget* cell = widget.cells[prev_param][evt_idx];
            GtkStyleContext* context = gtk_widget_get_style_context(cell);
            gtk_style_context_remove_class(context, "multi-selected");
          }
        }
      }
    }
  }

  // Add selection to new cell
  if (state.selected_seq && state.selected_event) {
    Seq_idx sel_seq = *state.selected_seq;
    Event_idx sel_evt = *state.selected_event;
    size_t sel_param = state_.selected_param_idx;

    if (sel_seq < sequencer_widgets_.size()) {
      const auto& widget = sequencer_widgets_[sel_seq];
      if (sel_param < widget.cells.size() && sel_evt < widget.cells[sel_param].size()) {
        GtkWidget* cell = widget.cells[sel_param][sel_evt];
        GtkStyleContext* context = gtk_widget_get_style_context(cell);
        gtk_style_context_add_class(context, "selected");
      }

      // Add multi-selection highlighting
      if (!state_.selected_event_range.empty()) {
        for (Event_idx evt_idx : state_.selected_event_range) {
          if (sel_param < widget.cells.size() && evt_idx < widget.cells[sel_param].size()) {
            GtkWidget* cell = widget.cells[sel_param][evt_idx];
            GtkStyleContext* context = gtk_widget_get_style_context(cell);

            // Use different class for multi-selected vs primary selected
            if (evt_idx != sel_evt) {
              gtk_style_context_add_class(context, "multi-selected");
            }
          }
        }
      }
    }
  }

  // Update tracking
  state_.last_selected_seq = state.selected_seq;
  state_.last_selected_event = state.selected_event;
  state_.last_selected_param = state_.selected_param_idx;
  state_.last_selected_event_range = state_.selected_event_range;
}

// Update a specific cell's value from the controller state
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_cell_value(Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  using Traits = Event_parameter_traits<Event_t>;

  // Bounds check
  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  const auto& widget = sequencer_widgets_[seq_idx];
  if (param_idx >= widget.cells.size() || event_idx >= widget.cells[param_idx].size()) {
    return;
  }

  // Get fresh state
  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  // Update cell value
  if (seq_idx < state.events.size() && event_idx < state.events[seq_idx].size()) {
    const auto& event = state.events[seq_idx][event_idx];
    std::string value_str = Traits::get_parameter_value(event, param_idx);
    GtkWidget* cell = widget.cells[param_idx][event_idx];
    gtk_entry_set_text(GTK_ENTRY(cell), value_str.c_str());
  }
}

// Increment or decrement a cell's numeric value
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::increment_cell_value(Seq_idx seq_idx, Event_idx event_idx, size_t param_idx, bool increment) {
  using Traits = Event_parameter_traits<Event_t>;

  // Get fresh state
  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  // Bounds check
  if (seq_idx >= state.events.size() || event_idx >= state.events[seq_idx].size()) {
    return;
  }

  try {
    // Get current value as string
    const auto& event = state.events[seq_idx][event_idx];
    std::string current_value = Traits::get_parameter_value(event, param_idx);

    // Try to parse as double
    char* end;
    double numeric_value = std::strtod(current_value.c_str(), &end);

    // Check if it's a valid number
    if (end == current_value.c_str() || (*end != '\0' && *end != ' ')) {
      // Not a numeric value, ignore
      return;
    }

    // Determine increment amount
    // Check if current value has a decimal point to decide on increment amount
    double delta = (current_value.find('.') != std::string::npos) ? 0.001 : 1.0;
    if (!increment) {
      delta = -delta;
    }

    numeric_value += delta;

    // Convert back to string
    char buffer[32];
    if (current_value.find('.') != std::string::npos) {
      // Float format
      snprintf(buffer, sizeof(buffer), "%.3f", numeric_value);
    } else {
      // Integer format
      snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(numeric_value));
    }
    std::string new_value(buffer);

    // Apply via mutate
    controller_.mutate(seq_idx, event_idx, [param_idx, &new_value](Event_t&& evt) {
      Traits::set_parameter_value(evt, param_idx, new_value);
      return std::move(evt);
    });

    // Update GUI
    update_cell_value(seq_idx, event_idx, param_idx);
  } catch (const std::exception& e) {
    // Silently ignore errors for non-numeric parameters
  }
}

// Clear multi-cell selection
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::clear_multi_selection() {
  if (state_.selected_event_range.empty()) {
    return; // Nothing to clear
  }

  auto sel_seq = controller_.selected_seq();
  if (!sel_seq || *sel_seq >= sequencer_widgets_.size()) {
    state_.selected_event_range.clear();
    return;
  }

  const auto& widget = sequencer_widgets_[*sel_seq];
  size_t param = state_.selected_param_idx;

  // Remove multi-selected class from all cells
  for (Event_idx evt_idx : state_.selected_event_range) {
    if (param < widget.cells.size() && evt_idx < widget.cells[param].size()) {
      GtkWidget* cell = widget.cells[param][evt_idx];
      GtkStyleContext* context = gtk_widget_get_style_context(cell);
      gtk_style_context_remove_class(context, "multi-selected");
    }
  }

  state_.selected_event_range.clear();
  state_.last_selected_event_range.clear();
  state_.state_dirty = true;
}

// GUI wrapper functions for controller actions

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select(Seq_idx seq_idx, Event_idx event_idx) {
  controller_.select(seq_idx, event_idx);
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_next_seq() {
  controller_.select_next_seq();
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_prev_seq() {
  controller_.select_prev_seq();
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_next_pos() {
  controller_.select_next_pos();
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_prev_pos() {
  controller_.select_prev_pos();
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_next_param() {
  using Traits = Event_parameter_traits<Event_t>;
  state_.selected_param_idx = (state_.selected_param_idx + 1) % Traits::parameter_count;
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_prev_param() {
  using Traits = Event_parameter_traits<Event_t>;
  if (state_.selected_param_idx == 0) {
    state_.selected_param_idx = Traits::parameter_count - 1;
  } else {
    state_.selected_param_idx--;
  }
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_extend_selection_left() {
  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (!sel_seq || !sel_evt) {
    return;
  }

  auto& state = state_.controller_state;
  if (*sel_seq >= state.sizes.size() || state.sizes[*sel_seq] == 0) {
    return;
  }

  Event_idx seq_size = state.sizes[*sel_seq];

  // Initialize selection if empty
  if (state_.selected_event_range.empty()) {
    state_.anchor_event = *sel_evt;
    state_.selected_event_range.insert(*sel_evt);
  }

  // Calculate previous event index with wraparound
  Event_idx prev_evt = (*sel_evt == 0) ? (seq_size - 1) : (*sel_evt - 1);

  // Toggle: if already selected, remove; otherwise add
  auto it = state_.selected_event_range.find(prev_evt);
  if (it != state_.selected_event_range.end()) {
    state_.selected_event_range.erase(it);
  } else {
    state_.selected_event_range.insert(prev_evt);
  }

  // Move controller selection to the new position
  controller_.select(*sel_seq, prev_evt);
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_extend_selection_right() {
  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (!sel_seq || !sel_evt) {
    return;
  }

  auto& state = state_.controller_state;
  if (*sel_seq >= state.sizes.size() || state.sizes[*sel_seq] == 0) {
    return;
  }

  Event_idx seq_size = state.sizes[*sel_seq];

  // Initialize selection if empty
  if (state_.selected_event_range.empty()) {
    state_.anchor_event = *sel_evt;
    state_.selected_event_range.insert(*sel_evt);
  }

  // Calculate next event index with wraparound
  Event_idx next_evt = (*sel_evt + 1) % seq_size;

  // Toggle: if already selected, remove; otherwise add
  auto it = state_.selected_event_range.find(next_evt);
  if (it != state_.selected_event_range.end()) {
    state_.selected_event_range.erase(it);
  } else {
    state_.selected_event_range.insert(next_evt);
  }

  // Move controller selection to the new position
  controller_.select(*sel_seq, next_evt);
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_all_in_row() {
  auto sel_seq = controller_.selected_seq();
  if (!sel_seq) {
    return;
  }

  auto& state = state_.controller_state;
  if (*sel_seq >= state.sizes.size() || state.sizes[*sel_seq] == 0) {
    return;
  }

  // Clear existing multi-selection
  state_.selected_event_range.clear();

  // Add all event indices to selection
  for (Event_idx i = 0; i < state.sizes[*sel_seq]; ++i) {
    state_.selected_event_range.insert(i);
  }

  // Mark state as dirty for re-render
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_start(Seq_idx seq_idx) {
  auto start_time = Controller::Clock::now() + std::chrono::milliseconds(50);
  controller_.start(seq_idx, start_time, true); // true for repeat
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_pause(Seq_idx seq_idx) {
  auto pause_time = Controller::Clock::now() + std::chrono::milliseconds(50);
  controller_.pause(seq_idx, pause_time);
  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_stop(Seq_idx seq_idx) {
  auto stop_time = Controller::Clock::now() + std::chrono::milliseconds(50);
  controller_.stop(seq_idx, stop_time, 0); // 0 = reset to position 0

  // Update playhead visual to position 0
  if (seq_idx < state_.sequencer_gui_states.size()) {
    auto& gui_state = state_.sequencer_gui_states[seq_idx];
    update_playhead_visual(seq_idx, gui_state.last_rendered_playhead, 0);
    gui_state.last_rendered_playhead = 0;
    gui_state.playhead_visible = true;
  }

  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_toggle_play(Seq_idx seq_idx) {
  if (controller_.is_scheduling(seq_idx)) {
    gui_pause(seq_idx);
  } else {
    gui_start(seq_idx);
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_toggle_sequencer() {
  auto sel_seq = controller_.selected_seq();
  if (!sel_seq) {
    show_error("No sequencer selected");
    return;
  }

  try {
    controller_.toggle_sequencer(*sel_seq);
    update_sequencer_header(*sel_seq);
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to toggle sequencer: ") + e.what());
  }
}

// Update window title with current mode
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_window_title() {
  std::string title = "MicroComposer - ";

  if (state_.in_tempo_multiply_mode) {
    title += "TEMPO MULTIPLY: " + state_.tempo_input_buffer;
  } else {
    title += (state_.mode == Mode::Normal) ? "NORMAL" : "EDIT";
  }

  gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
}

// Update sequencer header label
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_sequencer_header(Seq_idx seq_idx) {
  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  // Get fresh state
  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  if (seq_idx >= state.sizes.size()) {
    return;
  }

  const size_t num_events = state.sizes[seq_idx];
  auto& widget = sequencer_widgets_[seq_idx];

  // Build header text
  std::string header_text = "Sequencer " + std::to_string(seq_idx);
  header_text += " [" + std::to_string(num_events) + " steps]";

  // Add play/pause indicator
  if (seq_idx < state.scheduling.size() && state.scheduling[seq_idx]) {
    header_text += " ▶";
  } else {
    header_text += " ⏸";
  }

  // Add mute indicator
  if (controller_.is_sequencer_toggled(seq_idx)) {
    header_text += " 🔇";
  }

  // Update the label
  gtk_label_set_text(GTK_LABEL(widget.header_label), header_text.c_str());
}

// Focus the currently selected cell
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::focus_selected_cell() {
  const auto& state = state_.controller_state;

  // Check if there's a valid selection
  if (!state.selected_seq || !state.selected_event) {
    return;
  }

  Seq_idx sel_seq = *state.selected_seq;
  Event_idx sel_evt = *state.selected_event;
  size_t sel_param = state_.selected_param_idx;

  // Check bounds
  if (sel_seq >= sequencer_widgets_.size()) {
    return;
  }

  const auto& widget = sequencer_widgets_[sel_seq];

  // Focus the currently selected parameter row for the selected event
  if (sel_param < widget.cells.size() &&
      sel_evt < widget.cells[sel_param].size()) {
    GtkWidget* selected_cell = widget.cells[sel_param][sel_evt];
    gtk_widget_grab_focus(selected_cell);
  }
}

// Add event to selected sequencer
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_add_event() {
  auto sel_seq = controller_.selected_seq();
  if (!sel_seq) {
    show_error("No sequencer selected");
    return;
  }

  try {
    // Create a default event
    Event_t new_event{};
    new_event.enabled = true;
    new_event.duration = std::chrono::milliseconds(100);

    debug::msg("[GUI] Adding new event to sequencer " +
               std::to_string(*sel_seq));
    // Add event to sequencer
    controller_.push_back_event(*sel_seq, new_event);
    debug::msg("[GUI] Successfully pushed back event  to " +
               std::to_string(*sel_seq));

    debug::msg("[GUI] Rebuilding sequencer widget " + std::to_string(*sel_seq));
    // Rebuild only the affected widget
    rebuild_sequencer_widget(*sel_seq);
    debug::msg("[GUI] Returned from rebuild_sequencer_widget()");

    // Mark state as dirty to trigger render
    state_.state_dirty = true;

    debug::msg("[GUI] Getting new size");
    // Select the newly added event
    auto new_size = controller_.get_state().sizes[*sel_seq];
    debug::msg("[GUI] Got new size: " + std::to_string(new_size));
    if (new_size > 0) {
      debug::msg("[GUI] selecting new event at index " +
                 std::to_string(new_size - 1));
      controller_.select(*sel_seq, new_size - 1);
      debug::msg("[GUI] Returned from controller_.select");
    }
  } catch (const std::exception& e) {
    show_error(std::string("Failed to add event: ") + e.what());
  }
}

// Remove last event from selected sequencer
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_remove_event() {
  auto sel_seq = controller_.selected_seq();
  if (!sel_seq) {
    show_error("No sequencer selected");
    return;
  }

  try {
    // Remove last event from sequencer
    controller_.pop_back_event(*sel_seq);

    // Rebuild only the affected widget
    rebuild_sequencer_widget(*sel_seq);

    // Mark state as dirty to trigger render
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to remove event: ") + e.what());
  }
}

// Clear selected sequencer with confirmation dialog
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_clear_sequence() {
  auto sel_seq = controller_.selected_seq();
  if (!sel_seq) {
    show_error("No sequencer selected");
    return;
  }

  // Create confirmation dialog
  GtkWidget* dialog = gtk_dialog_new_with_buttons(
      ("Clear Sequence " + std::to_string(*sel_seq) + "?").c_str(),
      GTK_WINDOW(window_),
      GTK_DIALOG_MODAL,
      "Cancel", GTK_RESPONSE_CANCEL,
      "OK", GTK_RESPONSE_OK,
      nullptr);

  // Run dialog and wait for response
  gint response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  // If user clicked OK or pressed Enter, clear the sequence
  if (response == GTK_RESPONSE_OK) {
    try {
      // Stop the sequencer first
      controller_.stop(*sel_seq,
                      Controller::Clock::now() + std::chrono::milliseconds(50),
                      0);

      // Clear all events from the sequencer
      controller_[*sel_seq].clear();

      // Rebuild the widget
      rebuild_sequencer_widget(*sel_seq);

      // Mark state as dirty
      state_.state_dirty = true;
    } catch (const std::exception& e) {
      show_error(std::string("Failed to clear sequence: ") + e.what());
    }
  }
}

// Tempo modification methods

// Multiply durations for a single sequencer
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_multiply_durations(Seq_idx seq_idx, double factor) {
  try {
    controller_.multiply_durations(seq_idx, factor);
    rebuild_sequencer_widget(seq_idx);
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to multiply durations: ") + e.what());
  }
}

// Multiply durations for all sequencers
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_multiply_durations_all(double factor) {
  try {
    controller_.multiply_durations_all(factor);
    // Rebuild all sequencer widgets
    for (Seq_idx i = 0; i < controller_.size(); ++i) {
      rebuild_sequencer_widget(i);
    }
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to multiply durations: ") + e.what());
  }
}

// Adjust durations for a single sequencer
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_adjust_durations(Seq_idx seq_idx, typename Controller::Duration delta, bool increment) {
  try {
    auto actual_delta = increment ? delta : -delta;
    controller_.adjust_durations(seq_idx, actual_delta);
    rebuild_sequencer_widget(seq_idx);
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to adjust durations: ") + e.what());
  }
}

// Adjust durations for all sequencers
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_adjust_durations_all(typename Controller::Duration delta, bool increment) {
  try {
    auto actual_delta = increment ? delta : -delta;
    controller_.adjust_durations_all(actual_delta);
    // Rebuild all sequencer widgets
    for (Seq_idx i = 0; i < controller_.size(); ++i) {
      rebuild_sequencer_widget(i);
    }
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to adjust durations: ") + e.what());
  }
}

// Enter tempo multiply mode
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_enter_tempo_multiply_mode() {
  state_.in_tempo_multiply_mode = true;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

// Apply tempo multiplication from input buffer
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_apply_tempo_multiply() {
  if (!state_.in_tempo_multiply_mode) {
    return;
  }

  // Parse the buffer as a double
  try {
    double factor = std::stod(state_.tempo_input_buffer);

    if (factor <= 0.0) {
      show_error("Tempo factor must be positive");
    } else {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        gui_multiply_durations(*sel_seq, factor);
      } else {
        show_error("No sequencer selected");
      }
    }
  } catch (const std::exception& e) {
    show_error(std::string("Invalid tempo factor: ") + e.what());
  }

  // Exit tempo multiply mode
  state_.in_tempo_multiply_mode = false;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

// Cancel tempo multiply mode
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_cancel_tempo_multiply() {
  state_.in_tempo_multiply_mode = false;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

// Build menu bar
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::build_menu_bar() {
  menu_bar_ = gtk_menu_bar_new();

  // Create File menu
  GtkWidget* file_menu = gtk_menu_new();
  GtkWidget* file_item = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_), file_item);

  // Add Save and Load items to File menu
  GtkWidget* save_item = gtk_menu_item_new_with_label("Save...");
  g_signal_connect(save_item, "activate", G_CALLBACK(on_save_activate), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);

  GtkWidget* load_item = gtk_menu_item_new_with_label("Load...");
  g_signal_connect(load_item, "activate", G_CALLBACK(on_load_activate), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), load_item);

  // Create Sequencer menu
  GtkWidget* sequencer_menu = gtk_menu_new();
  GtkWidget* sequencer_item = gtk_menu_item_new_with_label("Sequencer");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(sequencer_item), sequencer_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_), sequencer_item);

  // Add tempo modification items
  GtkWidget* multiply_tempo_item = gtk_menu_item_new_with_label("Multiply Tempo... (*)");
  g_signal_connect(multiply_tempo_item, "activate",
                   G_CALLBACK(+[](GtkMenuItem*, gpointer data) {
                     auto* gui = static_cast<Gui*>(data);
                     gui->gui_enter_tempo_multiply_mode();
                   }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(sequencer_menu), multiply_tempo_item);

  GtkWidget* inc_tempo_item = gtk_menu_item_new_with_label("Increment Tempo (>)");
  g_signal_connect(inc_tempo_item, "activate",
                   G_CALLBACK(+[](GtkMenuItem*, gpointer data) {
                     auto* gui = static_cast<Gui*>(data);
                     auto sel_seq = gui->controller_.selected_seq();
                     if (sel_seq) {
                       constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
                       gui->gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, true);
                     }
                   }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(sequencer_menu), inc_tempo_item);

  GtkWidget* dec_tempo_item = gtk_menu_item_new_with_label("Decrement Tempo (<)");
  g_signal_connect(dec_tempo_item, "activate",
                   G_CALLBACK(+[](GtkMenuItem*, gpointer data) {
                     auto* gui = static_cast<Gui*>(data);
                     auto sel_seq = gui->controller_.selected_seq();
                     if (sel_seq) {
                       constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
                       gui->gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, false);
                     }
                   }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(sequencer_menu), dec_tempo_item);

  // Create Help menu
  GtkWidget* help_menu = gtk_menu_new();
  GtkWidget* help_item = gtk_menu_item_new_with_label("Help");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_), help_item);

  // Add "Keyboard Shortcuts" item with F1 accelerator
  GtkWidget* shortcuts_item = gtk_menu_item_new_with_label("Keyboard Shortcuts");
  g_signal_connect(shortcuts_item, "activate", G_CALLBACK(on_help_activate), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), shortcuts_item);

  // Add menu bar to main vbox at the top
  gtk_box_pack_start(GTK_BOX(main_vbox_), menu_bar_, FALSE, FALSE, 0);
}

// Help menu callback
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_help_activate(GtkMenuItem* item, gpointer user_data) {
  (void)item; // Unused
  auto* gui = static_cast<Gui*>(user_data);
  gui->show_help_dialog();
}

// Show help dialog with keyboard shortcuts
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::show_help_dialog() {
  GtkWidget* dialog = gtk_dialog_new_with_buttons(
      "Keyboard Shortcuts",
      GTK_WINDOW(window_),
      GTK_DIALOG_MODAL,
      "OK",
      GTK_RESPONSE_OK,
      nullptr);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 600);

  GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  // Create scrolled window for help text
  GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(content_area), scrolled, TRUE, TRUE, 10);

  // Create text view for help content
  GtkWidget* text_view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 10);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 10);
  gtk_container_add(GTK_CONTAINER(scrolled), text_view);

  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  std::string help_text = R"(MicroComposer Keyboard Shortcuts

MODES
  i              Enter edit mode (allows editing cell values)
  ESC            Exit edit mode (return to normal mode)

NAVIGATION
  h / l          Navigate left/right between events
  j / k          Navigate down/up between parameters
  Ctrl+J / Ctrl+K  Navigate down/up between sequencers
  1-8            Jump to event at index 0-7

PLAYBACK
  SPACE          Toggle play/pause for selected sequencer
  t              Toggle (mute/unmute) selected sequencer
                 (playhead continues, but no output)
  Ctrl+SPACE     Toggle play/pause for ALL sequencers
  Ctrl+0 / Ctrl+)  Reset selected sequencer to position 0

EDITING
  ENTER (normal)   Toggle step enabled/disabled
  ENTER (edit)     Confirm edit and exit edit mode
  UP / DOWN        Increment/decrement numeric values
  Shift+H / Shift+L  Extend multi-selection left/right
  Ctrl+R           Select all cells in current row
  Ctrl+A           Add new event to selected sequencer
  Ctrl+D           Remove last event from selected sequencer
  Ctrl+C           Clear all events (with confirmation)

TEMPO
  *              Enter tempo multiply mode (type factor, press ENTER)
  >              Increment selected sequencer tempo by 10ms
  <              Decrement selected sequencer tempo by 10ms
  Ctrl+>         Increment ALL sequencers tempo by 10ms
  Ctrl+<         Decrement ALL sequencers tempo by 10ms

HELP
  F1             Show this help dialog

TIPS
  - Multi-selection: Use Shift+H/L to select multiple cells
  - Edits in multi-selection apply to all selected cells simultaneously
  - Navigation wraps around at sequence boundaries
  - Increment/decrement only works on numeric parameters)";

  gtk_text_buffer_set_text(buffer, help_text.c_str(), -1);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

// Get config file path (~/.config/microcomposer/config.txt)
template <sequencable::Mut_seq_event Event_t>
std::string Gui<Event_t>::get_config_file_path() const {
  const char* home = getenv("HOME");
  if (!home) {
    home = getpwuid(getuid())->pw_dir;
  }
  std::string config_dir = std::string(home) + "/.config/microcomposer";
  return config_dir + "/config.txt";
}

// Load preferences from config file
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::load_preferences() {
  std::string config_path = get_config_file_path();
  std::ifstream config_file(config_path);

  if (!config_file.is_open()) {
    // Config file doesn't exist, use defaults
    return;
  }

  std::string line;
  while (std::getline(config_file, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Parse key=value pairs
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    // Trim whitespace
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    // Apply preferences
    if (key == "fps") {
      preferences_.fps = std::stoi(value);
    } else if (key == "window_width") {
      preferences_.window_width = std::stoi(value);
    } else if (key == "window_height") {
      preferences_.window_height = std::stoi(value);
    } else if (key == "last_save_directory") {
      preferences_.last_save_directory = value;
    }
  }

  config_file.close();
}

// Save preferences to config file
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::save_preferences() {
  std::string config_path = get_config_file_path();

  // Create config directory if it doesn't exist
  const char* home = getenv("HOME");
  if (!home) {
    home = getpwuid(getuid())->pw_dir;
  }
  std::string config_dir = std::string(home) + "/.config/microcomposer";
  mkdir(config_dir.c_str(), 0755);

  std::ofstream config_file(config_path);
  if (!config_file.is_open()) {
    show_error("Failed to save preferences");
    return;
  }

  // Write preferences
  config_file << "# MicroComposer Configuration File\n";
  config_file << "# This file is automatically generated\n\n";
  config_file << "fps=" << preferences_.fps << "\n";
  config_file << "window_width=" << preferences_.window_width << "\n";
  config_file << "window_height=" << preferences_.window_height << "\n";
  if (!preferences_.last_save_directory.empty()) {
    config_file << "last_save_directory=" << preferences_.last_save_directory << "\n";
  }

  config_file.close();
}

// Apply preferences to GUI
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::apply_preferences() {
  // Apply FPS setting
  fps_ = preferences_.fps;
  frame_duration_ = Duration{1.0 / static_cast<double>(fps_)};

  // Apply window size (if window exists)
  if (window_) {
    gtk_window_set_default_size(GTK_WINDOW(window_),
                                 preferences_.window_width,
                                 preferences_.window_height);
  }
}

// Serialize sequences to JSON string
template <sequencable::Mut_seq_event Event_t>
std::string Gui<Event_t>::sequences_to_json() const {
  using Traits = Event_parameter_traits<Event_t>;
  const auto& state = state_.controller_state;

  std::ostringstream json;
  json << "{\n  \"sequencers\": [\n";

  for (size_t seq_idx = 0; seq_idx < state.events.size(); ++seq_idx) {
    if (seq_idx > 0) json << ",\n";
    json << "    {\n";
    json << "      \"events\": [\n";

    const auto& events = state.events[seq_idx];
    for (size_t evt_idx = 0; evt_idx < events.size(); ++evt_idx) {
      if (evt_idx > 0) json << ",\n";
      json << "        {\n";

      const auto& event = events[evt_idx];
      constexpr size_t num_params = Traits::parameter_count;

      for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
        if (param_idx > 0) json << ",\n";
        std::string param_name = Traits::get_parameter_name(param_idx);
        std::string param_value = Traits::get_parameter_value(event, param_idx);

        // Escape quotes in value
        size_t pos = 0;
        while ((pos = param_value.find('"', pos)) != std::string::npos) {
          param_value.insert(pos, "\\");
          pos += 2;
        }

        json << "          \"" << param_name << "\": \"" << param_value << "\"";
      }

      json << "\n        }";
    }

    json << "\n      ]\n";
    json << "    }";
  }

  json << "\n  ]\n}\n";
  return json.str();
}

// Deserialize sequences from JSON string
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::json_to_sequences(const std::string& json_str) {
  using Traits = Event_parameter_traits<Event_t>;

  // Simple JSON parser - parse line by line looking for parameter:value pairs
  std::istringstream input(json_str);
  std::string line;

  std::vector<std::vector<Event_t>> new_sequences;
  std::vector<Event_t> current_sequence;
  Event_t current_event{};
  size_t param_idx = 0;
  bool in_event = false;

  while (std::getline(input, line)) {
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    // Start of event object
    if (line == "{" && !in_event) {
      in_event = true;
      current_event = Event_t{};
      param_idx = 0;
      continue;
    }

    // End of event object
    if (line == "}," || line == "}") {
      if (in_event) {
        current_sequence.push_back(current_event);
        in_event = false;
      }
      continue;
    }

    // Look for parameter:value pairs
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos && in_event) {
      // Extract parameter name and value
      std::string param_part = line.substr(0, colon_pos);
      std::string value_part = line.substr(colon_pos + 1);

      // Remove quotes and whitespace
      auto remove_quotes = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\""));
        s.erase(s.find_last_not_of(" \t\",") + 1);
      };

      remove_quotes(param_part);
      remove_quotes(value_part);

      // Find parameter index by name
      constexpr size_t num_params = Traits::parameter_count;
      for (size_t i = 0; i < num_params; ++i) {
        if (Traits::get_parameter_name(i) == param_part) {
          try {
            Traits::set_parameter_value(current_event, i, value_part);
          } catch (const std::exception& e) {
            show_error(std::string("Error parsing parameter '") + param_part +
                      "': " + e.what());
            return;
          }
          break;
        }
      }
    }

    // Check for start of new sequencer
    if (line.find("\"events\":") != std::string::npos && !current_sequence.empty()) {
      new_sequences.push_back(current_sequence);
      current_sequence.clear();
    }
  }

  // Add last sequence if any
  if (!current_sequence.empty()) {
    new_sequences.push_back(current_sequence);
  }

  // Clear all existing sequences
  for (size_t seq_idx = 0; seq_idx < controller_.size(); ++seq_idx) {
    controller_[seq_idx].clear();
  }

  // Load new sequences
  for (size_t seq_idx = 0; seq_idx < new_sequences.size() && seq_idx < controller_.size(); ++seq_idx) {
    for (const auto& event : new_sequences[seq_idx]) {
      controller_.push_back_event(seq_idx, event);
    }
    rebuild_sequencer_widget(seq_idx);
  }

  state_.state_dirty = true;
}

// Save sequences to file
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::save_sequences_to_file(const std::string& filepath) {
  std::string json_str = sequences_to_json();

  std::ofstream file(filepath);
  if (!file.is_open()) {
    show_error("Failed to open file for writing: " + filepath);
    return;
  }

  file << json_str;
  file.close();

  // Update last save directory in preferences
  size_t last_slash = filepath.find_last_of('/');
  if (last_slash != std::string::npos) {
    preferences_.last_save_directory = filepath.substr(0, last_slash);
    save_preferences();
  }
}

// Load sequences from file
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::load_sequences_from_file(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    show_error("Failed to open file for reading: " + filepath);
    return;
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  file.close();

  json_to_sequences(buffer.str());

  // Update last save directory in preferences
  size_t last_slash = filepath.find_last_of('/');
  if (last_slash != std::string::npos) {
    preferences_.last_save_directory = filepath.substr(0, last_slash);
    save_preferences();
  }
}

// Save menu callback
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_save_activate(GtkMenuItem* item, gpointer user_data) {
  (void)item;
  auto* gui = static_cast<Gui*>(user_data);

  GtkWidget* dialog = gtk_file_chooser_dialog_new(
      "Save Sequences",
      GTK_WINDOW(gui->window_),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      "Cancel", GTK_RESPONSE_CANCEL,
      "Save", GTK_RESPONSE_ACCEPT,
      nullptr);

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

  // Set default directory from preferences
  if (!gui->preferences_.last_save_directory.empty()) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        gui->preferences_.last_save_directory.c_str());
  }

  // Set default filename
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "sequences.json");

  gint result = gtk_dialog_run(GTK_DIALOG(dialog));

  if (result == GTK_RESPONSE_ACCEPT) {
    char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gui->save_sequences_to_file(std::string(filename));
    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

// Load menu callback
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_load_activate(GtkMenuItem* item, gpointer user_data) {
  (void)item;
  auto* gui = static_cast<Gui*>(user_data);

  GtkWidget* dialog = gtk_file_chooser_dialog_new(
      "Load Sequences",
      GTK_WINDOW(gui->window_),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      "Cancel", GTK_RESPONSE_CANCEL,
      "Load", GTK_RESPONSE_ACCEPT,
      nullptr);

  // Set default directory from preferences
  if (!gui->preferences_.last_save_directory.empty()) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        gui->preferences_.last_save_directory.c_str());
  }

  gint result = gtk_dialog_run(GTK_DIALOG(dialog));

  if (result == GTK_RESPONSE_ACCEPT) {
    char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gui->load_sequences_from_file(std::string(filename));
    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

} // namespace gui

} // namespace Micro_composer
