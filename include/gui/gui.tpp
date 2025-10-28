#include "gui/gui.h"
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

  normal_mode_actions_[GDK_KEY_j] = [this]() { gui_select_next_seq(); };

  normal_mode_actions_[GDK_KEY_k] = [this]() { gui_select_prev_seq(); };

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
  gtk_window_set_default_size(GTK_WINDOW(window_), 800, 600);

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

  // Create scrolled window
  GtkWidget* scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_container_add(GTK_CONTAINER(window_), scrolled_window);

  // Create grid
  grid_ = gtk_grid_new();
  gtk_widget_set_name(grid_, "sequencer-grid");
  gtk_container_add(GTK_CONTAINER(scrolled_window), grid_);

  build_grid();
}

// Build the grid of entry widgets
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::build_grid() {
  const auto& state = state_.controller_state;

  // Clear existing grid
  cell_entries_.clear();

  // Build grid for each sequencer
  for (Seq_idx seq_idx = 0; seq_idx < state.sizes.size(); ++seq_idx) {
    std::vector<GtkWidget*> row;
    for (Event_idx evt_idx = 0; evt_idx < state.sizes[seq_idx]; ++evt_idx) {
      GtkWidget* entry = gtk_entry_new();
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);

      // Set initial text based on event data
      if (seq_idx < state.events.size() &&
          evt_idx < state.events[seq_idx].size()) {
        const auto& event = state.events[seq_idx][evt_idx];
        // Format event as string (you'll need to customize this based on
        // Event_t)
        gtk_entry_set_text(GTK_ENTRY(entry),
                           std::to_string(evt_idx).c_str());
      }

      gtk_grid_attach(GTK_GRID(grid_), entry, evt_idx, seq_idx, 1, 1);
      row.push_back(entry);
    }
    cell_entries_.push_back(row);
  }
}

// GTK key press callback
template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_key_press(GtkWidget* widget, GdkEventKey* event,
                                    gpointer user_data) {
  auto* gui = static_cast<Gui*>(user_data);

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
  if (seq_idx >= cell_entries_.size()) {
    return;
  }

  const auto& row = cell_entries_[seq_idx];

  // Remove playhead class from old cell
  if (old_pos < row.size()) {
    GtkWidget* old_cell = row[old_pos];
    GtkStyleContext* context = gtk_widget_get_style_context(old_cell);
    gtk_style_context_remove_class(context, "playhead");
  }

  // Add playhead class to new cell
  if (new_pos < row.size()) {
    GtkWidget* new_cell = row[new_pos];
    GtkStyleContext* context = gtk_widget_get_style_context(new_cell);
    gtk_style_context_add_class(context, "playhead");
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
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::render_grid() {
  const auto& state = state_.controller_state;

  // Update selection visual
  if (state.selected_seq && state.selected_event) {
    Seq_idx sel_seq = *state.selected_seq;
    Event_idx sel_evt = *state.selected_event;

    // Remove all selection classes first
    for (auto& row : cell_entries_) {
      for (auto* cell : row) {
        GtkStyleContext* context = gtk_widget_get_style_context(cell);
        gtk_style_context_remove_class(context, "selected");
      }
    }

    // Add selection class to selected cell
    if (sel_seq < cell_entries_.size() &&
        sel_evt < cell_entries_[sel_seq].size()) {
      GtkWidget* selected_cell = cell_entries_[sel_seq][sel_evt];
      GtkStyleContext* context = gtk_widget_get_style_context(selected_cell);
      gtk_style_context_add_class(context, "selected");
    }
  }
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

} // namespace gui

} // namespace Micro_composer
