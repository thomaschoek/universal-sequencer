#include "gui/event_parameter_traits.h"
#include "gui/gui.h"
#include "utility/debug.h"
#include <cstring>
#include <gdk/gdkkeysyms.h>
#include <iostream>

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

  normal_mode_actions_[GDK_KEY_i] = [this]() {
    state_.mode = Mode::Edit;
    state_.state_dirty = true;
    update_window_title();
    focus_selected_cell();
  };

  // Edit mode mappings
  edit_mode_actions_[GDK_KEY_Escape] = [this]() {
    state_.mode = Mode::Normal;
    state_.state_dirty = true;
    update_window_title();
  };

  // Initialize GTK
  gtk_init(nullptr, nullptr);
  init_widgets();
  update_window_title();
}

// Destructor
template <sequencable::Mut_seq_event Event_t> Gui<Event_t>::~Gui() {
  if (window_) {
    gtk_widget_destroy(window_);
  }
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
  main_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window_), main_vbox_);

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

  // Try to apply the edit
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

// Entry activate callback - apply edit when Enter is pressed
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_activate(GtkEntry* entry, gpointer user_data) {
  auto* data = static_cast<Entry_user_data*>(user_data);
  auto* gui = data->gui;

  // Get the entered text
  const char* text = gtk_entry_get_text(entry);
  std::string value_str(text);

  // Try to apply the edit
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

// GTK key press callback
template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_key_press(GtkWidget* widget, GdkEventKey* event,
                                    gpointer user_data) {
  auto* gui = static_cast<Gui*>(user_data);

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
    }
  }

  // Update tracking
  state_.last_selected_seq = state.selected_seq;
  state_.last_selected_event = state.selected_event;
  state_.last_selected_param = state_.selected_param_idx;
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

// Update window title with current mode
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_window_title() {
  std::string title = "MicroComposer - ";
  title += (state_.mode == Mode::Normal) ? "NORMAL" : "EDIT";
  gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
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

} // namespace gui

} // namespace Micro_composer
