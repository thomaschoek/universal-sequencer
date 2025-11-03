#include "gui/event_parameter_traits.h"
#include "gui/gui.h"
#include "utility/debug.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <pwd.h>
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

  // 't' key toggles individual event enabled/disabled
  normal_mode_actions_[GDK_KEY_t] = [this]() {
    auto sel_seq = controller_.selected_seq();
    auto sel_evt = controller_.selected_event();
    if (sel_seq && sel_evt) {
      controller_.toggle(*sel_seq, *sel_evt);
      update_cell_value(*sel_seq, *sel_evt, 0);
      state_.state_dirty = true;
    }
  };

  // 'i' key enters edit mode
  normal_mode_actions_[GDK_KEY_i] = [this]() {
    state_.mode = Mode::Edit;
    state_.state_dirty = true;
    update_window_title();
    focus_selected_cell();
  };

  // 'Enter' key also enters edit mode
  normal_mode_actions_[GDK_KEY_Return] = [this]() {
    state_.mode = Mode::Edit;
    state_.state_dirty = true;
    update_window_title();
    focus_selected_cell();
  };

  // Keys 1-8 select events at indexes 0-7
  for (guint key = GDK_KEY_1; key <= GDK_KEY_8; ++key) {
    normal_mode_actions_[key] = [this, key]() {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        Event_idx event_idx = key - GDK_KEY_1;
        auto& state = state_.controller_state;
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
    // Apply edits from focused cell before exiting
    auto focused = window_->get_focus();
    if (focused) {
      Gtk::Entry* entry = dynamic_cast<Gtk::Entry*>(focused);
      if (entry) {
        // The focus-out handler will apply the edit
      }
    }

    // Exit edit mode
    state_.mode = Mode::Normal;
    clear_multi_selection();
    state_.state_dirty = true;
    update_window_title();

    // Clear text cursor by removing focus from entry widgets
    window_->set_focus(*window_);
  };

  // Load preferences
  load_preferences();

  // Create GTK application
  app_ = Gtk::Application::create("com.microcomposer.app");

  init_widgets();
  apply_preferences();
  update_window_title();
}

// Destructor
template <sequencable::Mut_seq_event Event_t> Gui<Event_t>::~Gui() {
  // Save current window size to preferences
  if (window_) {
    window_->get_default_size(preferences_.window_width, preferences_.window_height);
  }

  // Save preferences to config file
  save_preferences();

  // Clean up dynamically allocated widgets
  for (auto& seq_widget : sequencer_widgets_) {
    if (seq_widget.frame) delete seq_widget.frame;
  }

  if (window_) delete window_;
}

// Run the GUI
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::run() {
  running_ = true;

  // Register the application first (required before setting menubar and adding windows)
  app_->register_application();

  // Now we can build the menu bar (requires registered application)
  build_menu_bar();

  // Add timeout for event loop ticks
  Glib::signal_timeout().connect(
    sigc::mem_fun(*this, &Gui::on_tick),
    static_cast<unsigned int>(frame_duration_.count() * 1000)
  );

  // Add the window to the application and show it
  app_->add_window(*window_);
  window_->show();

  // Run the application main loop
  app_->run();
}

// Initialize GTK widgets
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::init_widgets() {
  // Create main window
  window_ = new Gtk::Window();
  window_->set_title("MicroComposer");
  window_->set_default_size(1000, 700);

  // Connect key press signal using EventControllerKey
  auto key_controller = Gtk::EventControllerKey::create();
  key_controller->signal_key_pressed().connect(
    sigc::mem_fun(*this, &Gui::on_key_press), false
  );
  window_->add_controller(key_controller);

  // Load CSS
  auto css_provider = Gtk::CssProvider::create();
  css_provider->load_from_path("resources/gui_style.css");
  Gtk::StyleContext::add_provider_for_display(
    window_->get_display(),
    css_provider,
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );

  // Create main vertical box
  main_vbox_ = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
  window_->set_child(*main_vbox_);

  // Note: build_menu_bar() will be called in run() after app registration

  // Create error label (initially hidden)
  error_label_ = Gtk::make_managed<Gtk::Label>("");
  error_label_->set_name("error-label");
  main_vbox_->append(*error_label_);
  error_label_->set_visible(false);

  // Create scrolled window for sequencer widgets
  scrolled_window_ = Gtk::make_managed<Gtk::ScrolledWindow>();
  main_vbox_->append(*scrolled_window_);
  scrolled_window_->set_vexpand(true);

  // Create vertical box to hold sequencer widgets
  sequencers_vbox_ = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
  scrolled_window_->set_child(*sequencers_vbox_);

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
      sequencers_vbox_->remove(*widget.frame);
      delete widget.frame;
    }
  }
  sequencer_widgets_.clear();

  constexpr size_t num_params = Traits::parameter_count;

  // Create a widget for each sequencer
  for (Seq_idx seq_idx = 0; seq_idx < state.sizes.size(); ++seq_idx) {
    Sequencer_widget widget;
    const size_t num_events = state.sizes[seq_idx];

    // Create frame
    widget.frame = new Gtk::Frame();
    widget.frame->set_name("sequencer-frame");

    // Create vertical box
    widget.vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    widget.frame->set_child(*widget.vbox);

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
    widget.header_label = Gtk::make_managed<Gtk::Label>(header_text);
    widget.header_label->set_name("sequencer-header");
    widget.vbox->append(*widget.header_label);

    // Create grid for column headers and parameters
    widget.grid = Gtk::make_managed<Gtk::Grid>();
    widget.grid->set_name("sequencer-grid");
    widget.grid->set_row_spacing(2);
    widget.grid->set_column_spacing(2);
    widget.vbox->append(*widget.grid);
    widget.grid->set_vexpand(true);

    // Create column header row (row 0)
    auto corner_label = Gtk::make_managed<Gtk::Label>("");
    corner_label->set_size_request(80, -1);
    widget.grid->attach(*corner_label, 0, 0, 1, 1);

    // Add column headers for each event
    for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
      std::string col_label = std::to_string(evt_idx);
      auto col_header_label = Gtk::make_managed<Gtk::Label>(col_label);
      col_header_label->set_size_request(70, -1);
      col_header_label->set_name("column-header");
      widget.grid->attach(*col_header_label, evt_idx + 1, 0, 1, 1);
    }

    // Build parameter rows (starting from row 1, row 0 is headers)
    widget.cells.resize(num_params);
    widget.row_labels.resize(num_params);

    for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
      // Create row label
      auto label = Gtk::make_managed<Gtk::Label>(Traits::get_parameter_name(param_idx));
      label->set_size_request(80, -1);
      label->set_halign(Gtk::Align::START);
      widget.grid->attach(*label, 0, param_idx + 1, 1, 1);
      widget.row_labels[param_idx] = label;

      // Create entry widgets for each event
      for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
        auto entry = Gtk::make_managed<Gtk::Entry>();
        entry->set_width_chars(10);
        entry->set_size_request(70, -1);

        // Set initial value
        if (seq_idx < state.events.size() &&
            evt_idx < state.events[seq_idx].size()) {
          const auto& event = state.events[seq_idx][evt_idx];
          std::string value_str = Traits::get_parameter_value(event, param_idx);
          entry->set_text(value_str);
        }

        // Connect signals with lambda captures
        auto focus_controller = Gtk::EventControllerFocus::create();
        focus_controller->signal_enter().connect([this, entry, seq_idx, evt_idx, param_idx]() {
          on_entry_focus_in(entry, seq_idx, evt_idx, param_idx);
        });
        focus_controller->signal_leave().connect([this, entry, seq_idx, evt_idx, param_idx]() {
          on_entry_focus_out(entry, seq_idx, evt_idx, param_idx);
        });
        entry->add_controller(focus_controller);

        entry->signal_activate().connect([this, entry, seq_idx, evt_idx, param_idx]() {
          on_entry_activate(entry, seq_idx, evt_idx, param_idx);
        });

        entry->signal_changed().connect([this, entry, seq_idx, evt_idx, param_idx]() {
          on_entry_changed(entry, seq_idx, evt_idx, param_idx);
        });

        auto scroll_controller = Gtk::EventControllerScroll::create();
        scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
        scroll_controller->signal_scroll().connect([this, entry, seq_idx, evt_idx, param_idx](double dx, double dy) {
          return on_entry_scroll(entry, dx, dy, seq_idx, evt_idx, param_idx);
        }, false);
        entry->add_controller(scroll_controller);

        widget.grid->attach(*entry, evt_idx + 1, param_idx + 1, 1, 1);
        widget.cells[param_idx].push_back(entry);
      }
    }

    // Add to container
    sequencers_vbox_->append(*widget.frame);
    sequencer_widgets_.push_back(widget);
  }
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
    sequencers_vbox_->remove(*old_widget.frame);
    delete old_widget.frame;
  }

  // Build new widget
  Sequencer_widget widget;
  const size_t num_events = state.sizes[seq_idx];
  constexpr size_t num_params = Traits::parameter_count;

  // Create frame
  widget.frame = new Gtk::Frame();
  widget.frame->set_name("sequencer-frame");

  // Create vertical box
  widget.vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
  widget.frame->set_child(*widget.vbox);

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
  widget.header_label = Gtk::make_managed<Gtk::Label>(header_text);
  widget.header_label->set_name("sequencer-header");
  widget.vbox->append(*widget.header_label);

  // Create grid for column headers and parameters
  widget.grid = Gtk::make_managed<Gtk::Grid>();
  widget.grid->set_name("sequencer-grid");
  widget.grid->set_row_spacing(2);
  widget.grid->set_column_spacing(2);
  widget.vbox->append(*widget.grid);
  widget.grid->set_vexpand(true);

  // Create column header row
  auto corner_label = Gtk::make_managed<Gtk::Label>("");
  corner_label->set_size_request(80, -1);
  widget.grid->attach(*corner_label, 0, 0, 1, 1);

  // Add column headers for each event
  for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
    std::string col_label = std::to_string(evt_idx);
    auto col_header_label = Gtk::make_managed<Gtk::Label>(col_label);
    col_header_label->set_size_request(70, -1);
    col_header_label->set_name("column-header");
    widget.grid->attach(*col_header_label, evt_idx + 1, 0, 1, 1);
  }

  // Build parameter rows
  widget.cells.resize(num_params);
  widget.row_labels.resize(num_params);

  for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
    auto label = Gtk::make_managed<Gtk::Label>(Traits::get_parameter_name(param_idx));
    label->set_size_request(80, -1);
    label->set_halign(Gtk::Align::START);
    widget.grid->attach(*label, 0, param_idx + 1, 1, 1);
    widget.row_labels[param_idx] = label;

    for (Event_idx evt_idx = 0; evt_idx < num_events; ++evt_idx) {
      auto entry = Gtk::make_managed<Gtk::Entry>();
      entry->set_width_chars(10);
      entry->set_size_request(70, -1);

      if (seq_idx < state.events.size() &&
          evt_idx < state.events[seq_idx].size()) {
        const auto& event = state.events[seq_idx][evt_idx];
        std::string value_str = Traits::get_parameter_value(event, param_idx);
        entry->set_text(value_str);
      }

      // Connect signals
      auto focus_controller = Gtk::EventControllerFocus::create();
      focus_controller->signal_enter().connect([this, entry, seq_idx, evt_idx, param_idx]() {
        on_entry_focus_in(entry, seq_idx, evt_idx, param_idx);
      });
      focus_controller->signal_leave().connect([this, entry, seq_idx, evt_idx, param_idx]() {
        on_entry_focus_out(entry, seq_idx, evt_idx, param_idx);
      });
      entry->add_controller(focus_controller);

      entry->signal_activate().connect([this, entry, seq_idx, evt_idx, param_idx]() {
        on_entry_activate(entry, seq_idx, evt_idx, param_idx);
      });

      entry->signal_changed().connect([this, entry, seq_idx, evt_idx, param_idx]() {
        on_entry_changed(entry, seq_idx, evt_idx, param_idx);
      });

      auto scroll_controller = Gtk::EventControllerScroll::create();
      scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
      scroll_controller->signal_scroll().connect([this, entry, seq_idx, evt_idx, param_idx](double dx, double dy) {
        return on_entry_scroll(entry, dx, dy, seq_idx, evt_idx, param_idx);
      }, false);
      entry->add_controller(scroll_controller);

      widget.grid->attach(*entry, evt_idx + 1, param_idx + 1, 1, 1);
      widget.cells[param_idx].push_back(entry);
    }
  }

  // Insert at correct position
  sequencers_vbox_->append(*widget.frame);

  // Replace in our vector
  sequencer_widgets_[seq_idx] = widget;
}

// Parse and apply edit to event parameter
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::parse_and_apply_edit(Seq_idx seq_idx, Event_idx event_idx,
                                        size_t param_idx,
                                        const std::string& value_str) {
  using Traits = Event_parameter_traits<Event_t>;

  try {
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
  if (error_timeout_connection_.connected()) {
    error_timeout_connection_.disconnect();
  }

  // Set error message and show
  error_label_->set_text(message);
  error_label_->set_visible(true);

  // Set timeout to clear after 5 seconds
  error_timeout_connection_ = Glib::signal_timeout().connect(
    sigc::mem_fun(*this, &Gui::clear_error_timeout),
    5000
  );
}

// Timeout callback to clear error message
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::clear_error_timeout() {
  error_label_->set_visible(false);
  return false; // Don't repeat
}

// Entry focus-in handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_focus_in(Gtk::Entry* entry, Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  // Sync our selection with GTK focus
  controller_.select(seq_idx, event_idx);
  state_.selected_param_idx = param_idx;
  state_.state_dirty = true;
}

// Entry focus-out handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_focus_out(Gtk::Entry* entry, Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  std::string value_str = entry->get_text();

  // Check if multi-selection is active
  if (!state_.selected_event_range.empty()) {
    bool all_success = true;
    for (Event_idx evt_idx : state_.selected_event_range) {
      bool success = parse_and_apply_edit(seq_idx, evt_idx, param_idx, value_str);
      all_success = all_success && success;
    }

    if (all_success) {
      for (Event_idx evt_idx : state_.selected_event_range) {
        update_cell_value(seq_idx, evt_idx, param_idx);
        if (seq_idx < sequencer_widgets_.size()) {
          auto& widget = sequencer_widgets_[seq_idx];
          if (param_idx < widget.cells.size() &&
              evt_idx < widget.cells[param_idx].size()) {
            widget.cells[param_idx][evt_idx]->queue_draw();
          }
        }
      }
    } else {
      using Traits = Event_parameter_traits<Event_t>;
      for (Event_idx evt_idx : state_.selected_event_range) {
        update_cell_value(seq_idx, evt_idx, param_idx);
        if (seq_idx < sequencer_widgets_.size()) {
          auto& widget = sequencer_widgets_[seq_idx];
          if (param_idx < widget.cells.size() &&
              evt_idx < widget.cells[param_idx].size()) {
            widget.cells[param_idx][evt_idx]->queue_draw();
          }
        }
      }
    }

    clear_multi_selection();
    state_.state_dirty = true;
  } else {
    bool success = parse_and_apply_edit(seq_idx, event_idx, param_idx, value_str);

    if (success) {
      state_.state_dirty = true;
    } else {
      using Traits = Event_parameter_traits<Event_t>;
      auto& state = state_.controller_state;
      if (seq_idx < state.events.size() &&
          event_idx < state.events[seq_idx].size()) {
        const auto& event = state.events[seq_idx][event_idx];
        std::string original_value =
            Traits::get_parameter_value(event, param_idx);
        entry->set_text(original_value);
      }
    }
  }
}

// Entry activate handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_activate(Gtk::Entry* entry, Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  std::string value_str = entry->get_text();

  if (!state_.selected_event_range.empty()) {
    bool all_success = true;
    for (Event_idx evt_idx : state_.selected_event_range) {
      bool success = parse_and_apply_edit(seq_idx, evt_idx, param_idx, value_str);
      all_success = all_success && success;
    }

    if (all_success) {
      for (Event_idx evt_idx : state_.selected_event_range) {
        update_cell_value(seq_idx, evt_idx, param_idx);
        if (seq_idx < sequencer_widgets_.size()) {
          auto& widget = sequencer_widgets_[seq_idx];
          if (param_idx < widget.cells.size() &&
              evt_idx < widget.cells[param_idx].size()) {
            widget.cells[param_idx][evt_idx]->queue_draw();
          }
        }
      }
      window_->set_focus(*window_);
    } else {
      using Traits = Event_parameter_traits<Event_t>;
      for (Event_idx evt_idx : state_.selected_event_range) {
        update_cell_value(seq_idx, evt_idx, param_idx);
        if (seq_idx < sequencer_widgets_.size()) {
          auto& widget = sequencer_widgets_[seq_idx];
          if (param_idx < widget.cells.size() &&
              evt_idx < widget.cells[param_idx].size()) {
            widget.cells[param_idx][evt_idx]->queue_draw();
          }
        }
      }
    }

    clear_multi_selection();
    state_.state_dirty = true;
  } else {
    bool success = parse_and_apply_edit(seq_idx, event_idx, param_idx, value_str);

    if (success) {
      state_.state_dirty = true;
      window_->set_focus(*window_);
    } else {
      using Traits = Event_parameter_traits<Event_t>;
      auto& state = state_.controller_state;
      if (seq_idx < state.events.size() &&
          event_idx < state.events[seq_idx].size()) {
        const auto& event = state.events[seq_idx][event_idx];
        std::string original_value =
            Traits::get_parameter_value(event, param_idx);
        entry->set_text(original_value);
      }
    }
  }
}

// Entry changed handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_entry_changed(Gtk::Entry* entry, Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  if (state_.in_text_update) {
    return;
  }

  state_.in_text_update = true;

  if (state_.selected_event_range.empty()) {
    state_.in_text_update = false;
    return;
  }

  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (!sel_seq || !sel_evt || event_idx != *sel_evt) {
    state_.in_text_update = false;
    return;
  }

  std::string text = entry->get_text();

  if (*sel_seq < sequencer_widgets_.size()) {
    auto& widget = sequencer_widgets_[*sel_seq];
    size_t param = state_.selected_param_idx;

    for (Event_idx evt_idx : state_.selected_event_range) {
      if (evt_idx != event_idx &&
          param < widget.cells.size() &&
          evt_idx < widget.cells[param].size()) {
        widget.cells[param][evt_idx]->set_text(text);
      }
    }
  }

  state_.in_text_update = false;
}

// Entry scroll handler
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::on_entry_scroll(Gtk::Entry* entry, double dx, double dy, Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  if (state_.mode != Mode::Normal) {
    return false;
  }

  bool increment = (dy < 0);
  increment_cell_value(seq_idx, event_idx, param_idx, increment);

  return true;
}

// GTK key press handler
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::on_key_press(guint keyval, guint keycode, Gdk::ModifierType state) {
  // Helper to check modifier
  auto has_modifier = [](Gdk::ModifierType state, Gdk::ModifierType mask) {
    return static_cast<bool>(state & mask);
  };

  // Check for F1 (help dialog) in any mode
  if (keyval == GDK_KEY_F1) {
    show_help_dialog();
    return true;
  }

  // Check for Ctrl+A (add event) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_a) {
    gui_add_event();
    return true;
  }

  // Check for Ctrl+D (remove event) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_d) {
    gui_remove_event();
    return true;
  }

  // Check for Ctrl+C (clear sequence) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_c) {
    gui_clear_sequence();
    return true;
  }

  // Check for Ctrl+J (next sequencer) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_j) {
    gui_select_next_seq();
    return true;
  }

  // Check for Ctrl+K (previous sequencer) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_k) {
    gui_select_prev_seq();
    return true;
  }

  // Check for Ctrl+R (select all in row) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) &&
      (keyval == GDK_KEY_r || keyval == GDK_KEY_R)) {
    gui_select_all_in_row();
    return true;
  }

  // Check for Ctrl+T (toggle sequencer mute) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) &&
      (keyval == GDK_KEY_t || keyval == GDK_KEY_T)) {
    gui_toggle_sequencer();
    return true;
  }

  // Check for Ctrl+Space (start/stop all sequencers) in any mode
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) && keyval == GDK_KEY_space) {
    auto time = Controller::Clock::now() + std::chrono::milliseconds(50);
    if (controller_.any_scheduling()) {
      controller_.pause(time);
    } else {
      controller_.start(time, true);
    }
    state_.state_dirty = true;
    return true;
  }

  // Check for Ctrl+0 (reset selected sequencer) or Ctrl+) (reset all sequencers)
  if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK) &&
      (keyval == GDK_KEY_0 || keyval == GDK_KEY_parenright)) {
    auto time = Controller::Clock::now() + std::chrono::milliseconds(50);

    if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) || keyval == GDK_KEY_parenright) {
      controller_.stop(time, 0);

      for (Seq_idx seq_idx = 0; seq_idx < state_.sequencer_gui_states.size(); ++seq_idx) {
        auto& gui_state = state_.sequencer_gui_states[seq_idx];
        update_playhead_visual(seq_idx, gui_state.last_rendered_playhead, 0);
        gui_state.last_rendered_playhead = 0;
        gui_state.playhead_visible = true;
      }
    } else {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        gui_stop(*sel_seq);
      }
    }

    state_.state_dirty = true;
    return true;
  }

  // Check for Shift+H (extend selection left) in normal mode
  if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) &&
      (keyval == GDK_KEY_h || keyval == GDK_KEY_H) &&
      state_.mode == Mode::Normal) {
    gui_extend_selection_left();
    return true;
  }

  // Check for Shift+L (extend selection right) in normal mode
  if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) &&
      (keyval == GDK_KEY_l || keyval == GDK_KEY_L) &&
      state_.mode == Mode::Normal) {
    gui_extend_selection_right();
    return true;
  }

  // Check for Shift+8 (asterisk) to enter tempo multiply mode
  if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) && keyval == GDK_KEY_8 &&
      state_.mode == Mode::Normal) {
    gui_enter_tempo_multiply_mode();
    return true;
  }

  // Handle tempo multiply mode input
  if (state_.in_tempo_multiply_mode) {
    if (keyval == GDK_KEY_Return) {
      gui_apply_tempo_multiply();
      return true;
    } else if (keyval == GDK_KEY_Escape) {
      gui_cancel_tempo_multiply();
      return true;
    } else if (keyval == GDK_KEY_BackSpace) {
      if (!state_.tempo_input_buffer.empty()) {
        state_.tempo_input_buffer.pop_back();
        update_window_title();
      }
      return true;
    } else if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
      state_.tempo_input_buffer += static_cast<char>('0' + (keyval - GDK_KEY_0));
      update_window_title();
      return true;
    } else if (keyval == GDK_KEY_period && !has_modifier(state, Gdk::ModifierType::SHIFT_MASK)) {
      state_.tempo_input_buffer += '.';
      update_window_title();
      return true;
    }
    return true;
  }

  // Check for Shift+period ('>') key (increment tempo)
  if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) && keyval == GDK_KEY_period) {
    constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
    if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK)) {
      gui_adjust_durations_all(TEMPO_INCREMENT_DELTA, true);
    } else {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, true);
      }
    }
    return true;
  }

  // Check for Shift+comma ('<') key (decrement tempo)
  if (has_modifier(state, Gdk::ModifierType::SHIFT_MASK) && keyval == GDK_KEY_comma) {
    constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
    if (has_modifier(state, Gdk::ModifierType::CONTROL_MASK)) {
      gui_adjust_durations_all(TEMPO_INCREMENT_DELTA, false);
    } else {
      auto sel_seq = controller_.selected_seq();
      if (sel_seq) {
        gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, false);
      }
    }
    return true;
  }

  if (state_.mode == Mode::Normal) {
    handle_normal_mode_key(keyval, state);
    return true;
  } else {
    if (keyval == GDK_KEY_Escape) {
      handle_edit_mode_key(keyval);
      return true;
    }
    return false;
  }
}

// Handle normal mode keyboard input
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::handle_normal_mode_key(guint keyval, Gdk::ModifierType state) {
  auto it = normal_mode_actions_.find(keyval);
  if (it != normal_mode_actions_.end()) {
    it->second();
  }
}

// Handle edit mode keyboard input
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::handle_edit_mode_key(guint keyval) {
  auto it = edit_mode_actions_.find(keyval);
  if (it != edit_mode_actions_.end()) {
    it->second();
  }
}

// Event loop tick callback
template <sequencable::Mut_seq_event Event_t>
bool Gui<Event_t>::on_tick() {
  if (!running_) {
    return false;
  }

  update_playheads();

  if (state_.state_dirty) {
    render();
    state_.state_dirty = false;
  }

  return true;
}

// Update playhead positions for running sequencers only
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_playheads() {
  auto current_state = controller_.get_state();

  for (Seq_idx seq_idx = 0; seq_idx < current_state.positions.size();
       ++seq_idx) {
    if (seq_idx >= state_.sequencer_gui_states.size()) {
      continue;
    }

    if (!current_state.scheduling[seq_idx]) {
      continue;
    }

    auto& gui_state = state_.sequencer_gui_states[seq_idx];
    Event_idx current_pos = current_state.positions[seq_idx];

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

  for (const auto& param_row : widget.cells) {
    if (old_pos < param_row.size()) {
      param_row[old_pos]->get_style_context()->remove_class("playhead");
    }

    if (new_pos < param_row.size()) {
      param_row[new_pos]->get_style_context()->add_class("playhead");
    }
  }
}

// Render the GUI
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::render() {
  state_.controller_state = controller_.get_state();
  render_grid();
}

// Render the grid
template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::render_grid() {
  const auto& state = state_.controller_state;

  bool selection_changed = (state.selected_seq != state_.last_selected_seq) ||
                           (state.selected_event != state_.last_selected_event) ||
                           (state_.selected_param_idx != state_.last_selected_param);

  if (!selection_changed) {
    return;
  }

  // Remove selection from previous cell
  if (state_.last_selected_seq && state_.last_selected_event && state_.last_selected_param) {
    Seq_idx prev_seq = *state_.last_selected_seq;
    Event_idx prev_evt = *state_.last_selected_event;
    size_t prev_param = *state_.last_selected_param;

    if (prev_seq < sequencer_widgets_.size()) {
      const auto& widget = sequencer_widgets_[prev_seq];
      if (prev_param < widget.cells.size() && prev_evt < widget.cells[prev_param].size()) {
        widget.cells[prev_param][prev_evt]->get_style_context()->remove_class("selected");
      }

      if (!state_.last_selected_event_range.empty()) {
        for (Event_idx evt_idx : state_.last_selected_event_range) {
          if (prev_param < widget.cells.size() && evt_idx < widget.cells[prev_param].size()) {
            widget.cells[prev_param][evt_idx]->get_style_context()->remove_class("multi-selected");
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
        widget.cells[sel_param][sel_evt]->get_style_context()->add_class("selected");
      }

      if (!state_.selected_event_range.empty()) {
        for (Event_idx evt_idx : state_.selected_event_range) {
          if (sel_param < widget.cells.size() && evt_idx < widget.cells[sel_param].size()) {
            if (evt_idx != sel_evt) {
              widget.cells[sel_param][evt_idx]->get_style_context()->add_class("multi-selected");
            }
          }
        }
      }
    }
  }

  state_.last_selected_seq = state.selected_seq;
  state_.last_selected_event = state.selected_event;
  state_.last_selected_param = state_.selected_param_idx;
  state_.last_selected_event_range = state_.selected_event_range;
}

// Update a specific cell's value from the controller state
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_cell_value(Seq_idx seq_idx, Event_idx event_idx, size_t param_idx) {
  using Traits = Event_parameter_traits<Event_t>;

  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  const auto& widget = sequencer_widgets_[seq_idx];
  if (param_idx >= widget.cells.size() || event_idx >= widget.cells[param_idx].size()) {
    return;
  }

  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  if (seq_idx < state.events.size() && event_idx < state.events[seq_idx].size()) {
    const auto& event = state.events[seq_idx][event_idx];
    std::string value_str = Traits::get_parameter_value(event, param_idx);
    widget.cells[param_idx][event_idx]->set_text(value_str);
  }
}

// Increment or decrement a cell's numeric value
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::increment_cell_value(Seq_idx seq_idx, Event_idx event_idx, size_t param_idx, bool increment) {
  using Traits = Event_parameter_traits<Event_t>;

  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  if (seq_idx >= state.events.size() || event_idx >= state.events[seq_idx].size()) {
    return;
  }

  try {
    const auto& event = state.events[seq_idx][event_idx];
    std::string current_value = Traits::get_parameter_value(event, param_idx);

    char* end;
    double numeric_value = std::strtod(current_value.c_str(), &end);

    if (end == current_value.c_str() || (*end != '\0' && *end != ' ')) {
      return;
    }

    double delta = (current_value.find('.') != std::string::npos) ? 0.001 : 1.0;
    if (!increment) {
      delta = -delta;
    }

    numeric_value += delta;

    char buffer[32];
    if (current_value.find('.') != std::string::npos) {
      snprintf(buffer, sizeof(buffer), "%.3f", numeric_value);
    } else {
      snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(numeric_value));
    }
    std::string new_value(buffer);

    controller_.mutate(seq_idx, event_idx, [param_idx, &new_value](Event_t&& evt) {
      Traits::set_parameter_value(evt, param_idx, new_value);
      return std::move(evt);
    });

    update_cell_value(seq_idx, event_idx, param_idx);
  } catch (const std::exception& e) {
    // Silently ignore errors for non-numeric parameters
  }
}

// Clear multi-cell selection
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::clear_multi_selection() {
  if (state_.selected_event_range.empty()) {
    return;
  }

  auto sel_seq = controller_.selected_seq();
  if (!sel_seq || *sel_seq >= sequencer_widgets_.size()) {
    state_.selected_event_range.clear();
    return;
  }

  const auto& widget = sequencer_widgets_[*sel_seq];
  size_t param = state_.selected_param_idx;

  for (Event_idx evt_idx : state_.selected_event_range) {
    if (param < widget.cells.size() && evt_idx < widget.cells[param].size()) {
      widget.cells[param][evt_idx]->get_style_context()->remove_class("multi-selected");
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

  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (sel_seq && sel_evt && *sel_seq < sequencer_widgets_.size()) {
    auto& widget = sequencer_widgets_[*sel_seq];
    size_t param_idx = state_.selected_param_idx;
    if (param_idx < widget.cells.size() && *sel_evt < widget.cells[param_idx].size()) {
      widget.cells[param_idx][*sel_evt]->grab_focus();
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_prev_pos() {
  controller_.select_prev_pos();
  state_.state_dirty = true;

  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (sel_seq && sel_evt && *sel_seq < sequencer_widgets_.size()) {
    auto& widget = sequencer_widgets_[*sel_seq];
    size_t param_idx = state_.selected_param_idx;
    if (param_idx < widget.cells.size() && *sel_evt < widget.cells[param_idx].size()) {
      widget.cells[param_idx][*sel_evt]->grab_focus();
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_select_next_param() {
  using Traits = Event_parameter_traits<Event_t>;
  state_.selected_param_idx = (state_.selected_param_idx + 1) % Traits::parameter_count;
  state_.state_dirty = true;

  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (sel_seq && sel_evt && *sel_seq < sequencer_widgets_.size()) {
    auto& widget = sequencer_widgets_[*sel_seq];
    size_t param_idx = state_.selected_param_idx;
    if (param_idx < widget.cells.size() && *sel_evt < widget.cells[param_idx].size()) {
      widget.cells[param_idx][*sel_evt]->grab_focus();
    }
  }
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

  auto sel_seq = controller_.selected_seq();
  auto sel_evt = controller_.selected_event();
  if (sel_seq && sel_evt && *sel_seq < sequencer_widgets_.size()) {
    auto& widget = sequencer_widgets_[*sel_seq];
    size_t param_idx = state_.selected_param_idx;
    if (param_idx < widget.cells.size() && *sel_evt < widget.cells[param_idx].size()) {
      widget.cells[param_idx][*sel_evt]->grab_focus();
    }
  }
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

  if (state_.selected_event_range.empty()) {
    state_.anchor_event = *sel_evt;
    state_.selected_event_range.insert(*sel_evt);
  }

  Event_idx prev_evt = (*sel_evt == 0) ? (seq_size - 1) : (*sel_evt - 1);

  auto it = state_.selected_event_range.find(prev_evt);
  if (it != state_.selected_event_range.end()) {
    state_.selected_event_range.erase(it);
  } else {
    state_.selected_event_range.insert(prev_evt);
  }

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

  if (state_.selected_event_range.empty()) {
    state_.anchor_event = *sel_evt;
    state_.selected_event_range.insert(*sel_evt);
  }

  Event_idx next_evt = (*sel_evt + 1) % seq_size;

  auto it = state_.selected_event_range.find(next_evt);
  if (it != state_.selected_event_range.end()) {
    state_.selected_event_range.erase(it);
  } else {
    state_.selected_event_range.insert(next_evt);
  }

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

  state_.selected_event_range.clear();

  for (Event_idx i = 0; i < state.sizes[*sel_seq]; ++i) {
    state_.selected_event_range.insert(i);
  }

  state_.state_dirty = true;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_start(Seq_idx seq_idx) {
  auto start_time = Controller::Clock::now() + std::chrono::milliseconds(50);
  controller_.start(seq_idx, start_time, true);
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
  controller_.stop(seq_idx, stop_time, 0);

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

  window_->set_title(title);
}

// Update sequencer header label
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_sequencer_header(Seq_idx seq_idx) {
  if (seq_idx >= sequencer_widgets_.size()) {
    return;
  }

  state_.controller_state = controller_.get_state();
  const auto& state = state_.controller_state;

  if (seq_idx >= state.sizes.size()) {
    return;
  }

  const size_t num_events = state.sizes[seq_idx];
  auto& widget = sequencer_widgets_[seq_idx];

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

  widget.header_label->set_text(header_text);
}

// Focus the currently selected cell
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::focus_selected_cell() {
  const auto& state = state_.controller_state;

  if (!state.selected_seq || !state.selected_event) {
    return;
  }

  Seq_idx sel_seq = *state.selected_seq;
  Event_idx sel_evt = *state.selected_event;
  size_t sel_param = state_.selected_param_idx;

  if (sel_seq >= sequencer_widgets_.size()) {
    return;
  }

  const auto& widget = sequencer_widgets_[sel_seq];

  if (sel_param < widget.cells.size() &&
      sel_evt < widget.cells[sel_param].size()) {
    widget.cells[sel_param][sel_evt]->grab_focus();
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
    Event_t new_event{};
    new_event.enabled = true;
    new_event.duration = std::chrono::milliseconds(100);

    debug::msg("[GUI] Adding new event to sequencer " +
               std::to_string(*sel_seq));
    controller_.push_back_event(*sel_seq, new_event);
    debug::msg("[GUI] Successfully pushed back event to " +
               std::to_string(*sel_seq));

    debug::msg("[GUI] Rebuilding sequencer widget " + std::to_string(*sel_seq));
    rebuild_sequencer_widget(*sel_seq);
    debug::msg("[GUI] Returned from rebuild_sequencer_widget()");

    state_.state_dirty = true;

    debug::msg("[GUI] Getting new size");
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
    controller_.pop_back_event(*sel_seq);
    rebuild_sequencer_widget(*sel_seq);
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
  auto dialog = Gtk::make_managed<Gtk::MessageDialog>(*window_,
    "Clear Sequence " + std::to_string(*sel_seq) + "?",
    false,
    Gtk::MessageType::QUESTION,
    Gtk::ButtonsType::OK_CANCEL,
    true);

  dialog->set_modal(true);

  dialog->signal_response().connect([this, dialog, sel_seq](int response) {
    if (response == Gtk::ResponseType::OK) {
      try {
        controller_.stop(*sel_seq,
                        Controller::Clock::now() + std::chrono::milliseconds(50),
                        0);

        controller_[*sel_seq].clear();

        rebuild_sequencer_widget(*sel_seq);

        state_.state_dirty = true;
      } catch (const std::exception& e) {
        show_error(std::string("Failed to clear sequence: ") + e.what());
      }
    }
    dialog->set_visible(false);
  });

  dialog->show();
}

// Tempo modification methods

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

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_multiply_durations_all(double factor) {
  try {
    controller_.multiply_durations_all(factor);
    for (Seq_idx i = 0; i < controller_.size(); ++i) {
      rebuild_sequencer_widget(i);
    }
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to multiply durations: ") + e.what());
  }
}

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

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_adjust_durations_all(typename Controller::Duration delta, bool increment) {
  try {
    auto actual_delta = increment ? delta : -delta;
    controller_.adjust_durations_all(actual_delta);
    for (Seq_idx i = 0; i < controller_.size(); ++i) {
      rebuild_sequencer_widget(i);
    }
    state_.state_dirty = true;
  } catch (const std::exception& e) {
    show_error(std::string("Failed to adjust durations: ") + e.what());
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_enter_tempo_multiply_mode() {
  state_.in_tempo_multiply_mode = true;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_apply_tempo_multiply() {
  if (!state_.in_tempo_multiply_mode) {
    return;
  }

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

  state_.in_tempo_multiply_mode = false;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::gui_cancel_tempo_multiply() {
  state_.in_tempo_multiply_mode = false;
  state_.tempo_input_buffer.clear();
  update_window_title();
}

// Build menu bar
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::build_menu_bar() {
  auto menu_bar = Gio::Menu::create();

  // File menu
  auto file_menu = Gio::Menu::create();
  file_menu->append("Save...", "app.save");
  file_menu->append("Load...", "app.load");
  menu_bar->append_submenu("File", file_menu);

  // Sequencer menu
  auto sequencer_menu = Gio::Menu::create();
  sequencer_menu->append("Multiply Tempo... (*)", "app.multiply_tempo");
  sequencer_menu->append("Increment Tempo (>)", "app.increment_tempo");
  sequencer_menu->append("Decrement Tempo (<)", "app.decrement_tempo");
  menu_bar->append_submenu("Sequencer", sequencer_menu);

  // Help menu
  auto help_menu = Gio::Menu::create();
  help_menu->append("Keyboard Shortcuts", "app.help");
  menu_bar->append_submenu("Help", help_menu);

  // Create actions
  app_->add_action("save", sigc::mem_fun(*this, &Gui::on_save_activate));
  app_->add_action("load", sigc::mem_fun(*this, &Gui::on_load_activate));
  app_->add_action("help", sigc::mem_fun(*this, &Gui::on_help_activate));
  app_->add_action("multiply_tempo", sigc::mem_fun(*this, &Gui::gui_enter_tempo_multiply_mode));

  app_->add_action("increment_tempo", [this]() {
    auto sel_seq = controller_.selected_seq();
    if (sel_seq) {
      constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
      gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, true);
    }
  });

  app_->add_action("decrement_tempo", [this]() {
    auto sel_seq = controller_.selected_seq();
    if (sel_seq) {
      constexpr auto TEMPO_INCREMENT_DELTA = std::chrono::milliseconds(10);
      gui_adjust_durations(*sel_seq, TEMPO_INCREMENT_DELTA, false);
    }
  });

  app_->set_menubar(menu_bar);
}

// Help menu handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_help_activate() {
  show_help_dialog();
}

// Show help dialog with keyboard shortcuts
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::show_help_dialog() {
  auto dialog = Gtk::make_managed<Gtk::MessageDialog>(*window_,
    "Keyboard Shortcuts",
    false,
    Gtk::MessageType::INFO,
    Gtk::ButtonsType::OK,
    true);

  dialog->set_secondary_text(R"(MicroComposer Keyboard Shortcuts

MODES
  i              Enter edit mode
  ESC            Exit edit mode

NAVIGATION
  h / l          Navigate left/right between events
  j / k          Navigate down/up between parameters
  Ctrl+J / Ctrl+K  Navigate down/up between sequencers
  1-8            Jump to event at index 0-7

PLAYBACK
  SPACE          Toggle play/pause for selected sequencer
  t              Toggle selected event enabled/disabled
  Ctrl+SPACE     Toggle play/pause for ALL sequencers
  Ctrl+0         Reset selected sequencer to position 0

EDITING
  ENTER          Enter edit mode / Confirm edit
  UP / DOWN      Increment/decrement numeric values
  Shift+H / Shift+L  Extend multi-selection left/right
  Ctrl+R         Select all cells in current row
  Ctrl+A         Add new event
  Ctrl+D         Remove last event
  Ctrl+C         Clear all events

TEMPO
  *              Enter tempo multiply mode
  >              Increment tempo
  <              Decrement tempo

HELP
  F1             Show this dialog)");

  dialog->set_modal(true);
  dialog->signal_response().connect([dialog](int) {
    dialog->set_visible(false);
  });
  dialog->show();
}

// Get config file path
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
    return;
  }

  std::string line;
  while (std::getline(config_file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

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
  fps_ = preferences_.fps;
  frame_duration_ = Duration{1.0 / static_cast<double>(fps_)};

  if (window_) {
    window_->set_default_size(preferences_.window_width,
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

  std::istringstream input(json_str);
  std::string line;

  std::vector<std::vector<Event_t>> new_sequences;
  std::vector<Event_t> current_sequence;
  Event_t current_event{};
  size_t param_idx = 0;
  bool in_event = false;

  while (std::getline(input, line)) {
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    if (line == "{" && !in_event) {
      in_event = true;
      current_event = Event_t{};
      param_idx = 0;
      continue;
    }

    if (line == "}," || line == "}") {
      if (in_event) {
        current_sequence.push_back(current_event);
        in_event = false;
      }
      continue;
    }

    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos && in_event) {
      std::string param_part = line.substr(0, colon_pos);
      std::string value_part = line.substr(colon_pos + 1);

      auto remove_quotes = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\""));
        s.erase(s.find_last_not_of(" \t\",") + 1);
      };

      remove_quotes(param_part);
      remove_quotes(value_part);

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

    if (line.find("\"events\":") != std::string::npos && !current_sequence.empty()) {
      new_sequences.push_back(current_sequence);
      current_sequence.clear();
    }
  }

  if (!current_sequence.empty()) {
    new_sequences.push_back(current_sequence);
  }

  for (size_t seq_idx = 0; seq_idx < controller_.size(); ++seq_idx) {
    controller_[seq_idx].clear();
  }

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

  size_t last_slash = filepath.find_last_of('/');
  if (last_slash != std::string::npos) {
    preferences_.last_save_directory = filepath.substr(0, last_slash);
    save_preferences();
  }
}

// Save menu handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_save_activate() {
  auto dialog = Gtk::make_managed<Gtk::FileChooserDialog>(*window_,
    "Save Sequences",
    Gtk::FileChooser::Action::SAVE);

  dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
  dialog->add_button("_Save", Gtk::ResponseType::OK);

  if (!preferences_.last_save_directory.empty()) {
    dialog->set_current_folder(Gio::File::create_for_path(preferences_.last_save_directory));
  }

  dialog->set_current_name("sequences.json");

  dialog->set_modal(true);
  dialog->signal_response().connect([this, dialog](int result) {
    if (result == Gtk::ResponseType::OK) {
      auto file = dialog->get_file();
      if (file) {
        save_sequences_to_file(file->get_path());
      }
    }
    dialog->set_visible(false);
  });
  dialog->show();
}

// Load menu handler
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_load_activate() {
  auto dialog = Gtk::make_managed<Gtk::FileChooserDialog>(*window_,
    "Load Sequences",
    Gtk::FileChooser::Action::OPEN);

  dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
  dialog->add_button("_Load", Gtk::ResponseType::OK);

  if (!preferences_.last_save_directory.empty()) {
    dialog->set_current_folder(Gio::File::create_for_path(preferences_.last_save_directory));
  }

  dialog->set_modal(true);
  dialog->signal_response().connect([this, dialog](int result) {
    if (result == Gtk::ResponseType::OK) {
      auto file = dialog->get_file();
      if (file) {
        load_sequences_from_file(file->get_path());
      }
    }
    dialog->set_visible(false);
  });
  dialog->show();
}

} // namespace gui

} // namespace Micro_composer
