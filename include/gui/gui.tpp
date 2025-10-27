#include "gui/gui.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Micro_composer {

namespace gui {

template <sequencable::Mut_seq_event Event_t>
Gui<Event_t>::Gui(std::shared_ptr<Controller> controller)
    : controller_(controller) {
  // GTK will be initialized in init()
  // Initialize first sequence as expanded
  expanded_seqs_[0] = true;
}

template <sequencable::Mut_seq_event Event_t> Gui<Event_t>::~Gui() {
  // GTK cleanup handled by gtk_main_quit if needed
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::init(int argc, char** argv) {
  // Initialize GTK
  gtk_init(&argc, &argv);

  // Load CSS from file
  GtkCssProvider* css_provider = gtk_css_provider_new();
  GError* error = nullptr;

  // Try loading from install location first, then fallback to source location
  const char* css_paths[] = {"./resources/gui_style.css",
                             "../resources/gui_style.css",
                             "../../resources/gui_style.css"};

  bool css_loaded = false;
  for (const char* path : css_paths) {
    if (gtk_css_provider_load_from_path(css_provider, path, &error)) {
      std::cout << "[INFO] Loaded CSS from: " << path << std::endl;
      css_loaded = true;
      break;
    }
    if (error) {
      std::cerr << "[WARNING] Failed to load CSS from " << path << ": "
                << error->message << std::endl;
      g_clear_error(&error);
    }
  }

  if (!css_loaded) {
    std::cerr << "[WARNING] Could not load CSS file, using default styling"
              << std::endl;
  }

  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);

  // Create main window
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window_), "Micro Composer - Step Sequencer");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1024, 768);

  // Connect destroy signal
  g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

  // Connect global keyboard handler for window
  g_signal_connect(window_, "key-press-event", G_CALLBACK(on_window_key_press),
                   this);

  // Create main container
  main_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window_), main_box_);

  // Create menu bar (will be packed at top of main_box)
  create_menu_bar();

  // Create scrolled window for grid
  scrolled_window_ = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(main_box_), scrolled_window_, TRUE, TRUE, 0);

  // Create grid with fixed name for CSS styling
  grid_ = gtk_grid_new();
  gtk_widget_set_name(grid_, "sequencer-grid");
  gtk_grid_set_row_spacing(GTK_GRID(grid_), 2);
  gtk_grid_set_column_spacing(GTK_GRID(grid_), 2);
  gtk_container_add(GTK_CONTAINER(scrolled_window_), grid_);

  std::cout << "[INFO] GUI initialized" << std::endl;
}

template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::show() {
  if (window_) {
    gtk_widget_show_all(window_);
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::create_menu_bar() {
  menu_bar_ = gtk_menu_bar_new();

  // File menu
  GtkWidget* file_menu = gtk_menu_new();
  GtkWidget* file_item = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);

  // File -> Save (Ctrl+S)
  GtkWidget* save_item =
      gtk_menu_item_new_with_label("Save                          Ctrl+S");
  g_signal_connect_swapped(save_item, "activate", G_CALLBACK(+[](Gui* gui) {
                             auto now = std::time(nullptr);
                             auto tm = *std::localtime(&now);
                             std::ostringstream filename;
                             filename << "sequences_" << (tm.tm_year + 1900)
                                      << std::setfill('0') << std::setw(2)
                                      << (tm.tm_mon + 1) << std::setw(2)
                                      << tm.tm_mday << "_" << std::setw(2)
                                      << tm.tm_hour << std::setw(2) << tm.tm_min
                                      << std::setw(2) << tm.tm_sec << ".json";
                             gui->save_to_json(filename.str());
                           }),
                           this);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);

  // File -> Load (Ctrl+L)
  GtkWidget* load_item =
      gtk_menu_item_new_with_label("Load                          Ctrl+L");
  g_signal_connect_swapped(
      load_item, "activate", G_CALLBACK(+[](Gui* gui) {
        // Simple file chooser dialog
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "Load Session", GTK_WINDOW(gui->window_),
            GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT, NULL);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
          char* filename =
              gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          gui->load_from_json(filename);
          g_free(filename);
        }

        gtk_widget_destroy(dialog);
      }),
      this);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), load_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_), file_item);

  // Edit menu
  GtkWidget* edit_menu = gtk_menu_new();
  GtkWidget* edit_item = gtk_menu_item_new_with_label("Edit");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);

  // Step operations
  GtkWidget* toggle_step_item =
      gtk_menu_item_new_with_label("Toggle Step               T");
  g_signal_connect_swapped(
      toggle_step_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->toggle_selected_step(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), toggle_step_item);

  GtkWidget* add_step_item =
      gtk_menu_item_new_with_label("Add Step                  Ctrl+A");
  g_signal_connect_swapped(
      add_step_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->add_step_to_selected(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), add_step_item);

  GtkWidget* insert_step_item =
      gtk_menu_item_new_with_label("Insert Step               Ctrl+I");
  g_signal_connect_swapped(
      insert_step_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->insert_step_before_selected(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), insert_step_item);

  GtkWidget* remove_step_item =
      gtk_menu_item_new_with_label("Remove Step               Ctrl+D");
  g_signal_connect_swapped(
      remove_step_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->remove_selected_step(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), remove_step_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                        gtk_separator_menu_item_new());

  // Sequence operations
  GtkWidget* add_seq_item =
      gtk_menu_item_new_with_label("Add Sequence              Ctrl+Shift+A");
  g_signal_connect_swapped(
      add_seq_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->add_new_sequence(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), add_seq_item);

  GtkWidget* remove_seq_item =
      gtk_menu_item_new_with_label("Remove Sequence           Ctrl+Shift+D");
  g_signal_connect_swapped(
      remove_seq_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->remove_selected_sequence(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), remove_seq_item);

  GtkWidget* dup_seq_item =
      gtk_menu_item_new_with_label("Duplicate Sequence        Ctrl+Shift+C");
  g_signal_connect_swapped(
      dup_seq_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->duplicate_selected_sequence(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), dup_seq_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                        gtk_separator_menu_item_new());

  // Expand/collapse operations
  GtkWidget* expand_all_item =
      gtk_menu_item_new_with_label("Expand All                Ctrl+E");
  g_signal_connect_swapped(
      expand_all_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->expand_all_sequences(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), expand_all_item);

  GtkWidget* collapse_all_item =
      gtk_menu_item_new_with_label("Collapse All              Ctrl+W");
  g_signal_connect_swapped(
      collapse_all_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->collapse_all_sequences(); }), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), collapse_all_item);

  GtkWidget* toggle_expand_item =
      gtk_menu_item_new_with_label("Toggle Sequence           Ctrl+T");
  g_signal_connect_swapped(
      toggle_expand_item, "activate",
      G_CALLBACK(+[](Gui* gui) { gui->toggle_selected_sequence_expand(); }),
      this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), toggle_expand_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                        gtk_separator_menu_item_new());

  // BPM editing
  GtkWidget* edit_bpm_item =
      gtk_menu_item_new_with_label("Edit BPM                  Ctrl+B");
  g_signal_connect_swapped(edit_bpm_item, "activate",
                           G_CALLBACK(+[](Gui* gui) { gui->edit_bpm(); }),
                           this);
  gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_bpm_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_), edit_item);

  // Pack menu bar at the top of main_box
  gtk_box_pack_start(GTK_BOX(main_box_), menu_bar_, FALSE, FALSE, 0);
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::render(const Controller_state& state) {
  // Skip rendering if state hasn't changed
  if (state == last_state_) {
    return;
  }

  // Check if we need to rebuild the entire grid (structure changed)
  bool need_rebuild = false;
  if (state.sequencers.size() != last_state_.sequencers.size()) {
    need_rebuild = true;
  } else {
    for (std::size_t i = 0; i < state.sequencers.size(); ++i) {
      if (state.sequencers[i].num_steps !=
              last_state_.sequencers[i].num_steps ||
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
    // Apply highlighting after rebuild
    update_playhead_highlighting(state);
    update_selection_highlighting(state);
    update_play_icons(state);
  } else {
    // Just update highlighting (not values - they only change when user edits)
    update_playhead_highlighting(state);
    update_selection_highlighting(state);
    update_play_icons(state);
  }

  // Scroll to selection if it changed
  if (state.selected_seq != last_state_.selected_seq ||
      state.selected_event != last_state_.selected_event ||
      state.selected_param_idx != last_state_.selected_param_idx) {
    scroll_to_selection(state);
  }

  last_state_ = state;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::rebuild_grid(const Controller_state& state) {
  std::cout << "[DEBUG] Rebuilding entire grid..." << std::endl;

  // Clear existing grid contents
  gtk_container_foreach(
      GTK_CONTAINER(grid_),
      [](GtkWidget* widget, gpointer) { gtk_widget_destroy(widget); }, nullptr);
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

    // Check if this sequence is expanded (use local state)
    bool is_expanded = expanded_seqs_[seq_idx];

    // Create sequence header row
    create_sequence_header(seq_idx, seq, current_row);
    current_row++;

    // If expanded, create parameter rows and duration row
    if (is_expanded) {
      for (std::size_t param_idx = 0; param_idx < seq.num_params; ++param_idx) {
        create_parameter_row(seq_idx, param_idx, seq, current_row);
        current_row++;
      }
      // Add duration row after all parameter rows
      create_duration_row(seq_idx, seq, current_row);
      current_row++;
    }
  }

  gtk_widget_show_all(grid_);
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::create_sequence_header(std::size_t seq_idx,
                                          const Controller& controller,
                                          int row) {
  // Expand/collapse button (column 0)
  bool is_expanded = expanded_seqs_[seq_idx];
  GtkWidget* expand_btn = gtk_button_new_with_label(is_expanded ? "▼" : "▶");
  gtk_widget_set_size_request(expand_btn, 30, 30);
  g_object_set_data(G_OBJECT(expand_btn), "seq_idx", GSIZE_TO_POINTER(seq_idx));
  g_signal_connect(expand_btn, "clicked", G_CALLBACK(on_expand_clicked), this);
  gtk_grid_attach(GTK_GRID(grid_), expand_btn, 0, row, 1, 1);

  // Play/pause/stop icon (column 1)
  const char* icon_text = "⏹"; // Stop
  if (controller.is_running) {
    icon_text = (controller.current_step_idx > 0) ? "▶" : "▶";
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

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::create_parameter_row(std::size_t seq_idx,
                                        std::size_t param_idx,
                                        const Controller& controller, int row) {
  // Parameter label (columns 0-2, merged)
  std::ostringstream param_label_ss;
  param_label_ss << "  Param " << param_idx;
  GtkWidget* param_label = gtk_label_new(param_label_ss.str().c_str());
  gtk_widget_set_halign(param_label, GTK_ALIGN_START);
  gtk_widget_set_size_request(param_label, 140, 30);
  gtk_grid_attach(GTK_GRID(grid_), param_label, 0, row, 3, 1);

  // Create cells for each step
  for (std::size_t step_idx = 0; step_idx < controller.num_steps; ++step_idx) {
    // Get parameter value
    std::string value = "";
    if (step_idx < controller.steps.size() &&
        param_idx < controller.steps[step_idx].param_values.size()) {
      value = controller.steps[step_idx].param_values[param_idx];
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
    g_object_set_data(G_OBJECT(entry), "cell_key", g_strdup(key.c_str()));

    // Connect edit signal (Enter key)
    g_signal_connect(entry, "activate", G_CALLBACK(on_cell_edited), this);

    // Connect focus-out signal (when user leaves the cell)
    g_signal_connect(entry, "focus-out-event", G_CALLBACK(on_cell_focus_out),
                     this);

    // Connect keyboard navigation
    g_signal_connect(entry, "key-press-event", G_CALLBACK(on_cell_key_press),
                     this);

    // Add to grid (column offset by 3 for fixed columns)
    gtk_grid_attach(GTK_GRID(grid_), entry, step_idx + 3, row, 1, 1);
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::create_duration_row(std::size_t seq_idx,
                                       const Controller& controller, int row) {
  // Duration label (columns 0-2, merged)
  GtkWidget* duration_label = gtk_label_new("  Duration");
  gtk_widget_set_halign(duration_label, GTK_ALIGN_START);
  gtk_widget_set_size_request(duration_label, 140, 30);
  gtk_grid_attach(GTK_GRID(grid_), duration_label, 0, row, 3, 1);

  // Create duration cells for each step
  for (std::size_t step_idx = 0; step_idx < controller.num_steps; ++step_idx) {
    // Get duration value
    std::string value = "0.25";
    if (step_idx < controller.steps.size()) {
      // Use the number_to_string_no_trailing_zeros helper
      std::ostringstream oss;
      oss << controller.steps[step_idx].duration_seconds;
      value = oss.str();

      // Remove trailing zeros
      if (value.find('.') != std::string::npos) {
        value.erase(value.find_last_not_of('0') + 1, std::string::npos);
        if (!value.empty() && value.back() == '.') {
          value.pop_back();
        }
      }
    }

    // Create entry widget
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), value.c_str());
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 8);
    gtk_widget_set_size_request(entry, 70, 30);

    // Store cell info for event handling (use SIZE_MAX for param_idx to
    // indicate duration)
    CellWidget cell_widget{entry, seq_idx, step_idx, SIZE_MAX};
    std::string key = make_cell_key(seq_idx, step_idx, SIZE_MAX);
    cell_widgets_[key] = cell_widget;

    // Attach to user data for callback
    g_object_set_data(G_OBJECT(entry), "cell_key", g_strdup(key.c_str()));

    // Connect edit signal (Enter key)
    g_signal_connect(entry, "activate", G_CALLBACK(on_cell_edited), this);

    // Connect focus-out signal (when user leaves the cell)
    g_signal_connect(entry, "focus-out-event", G_CALLBACK(on_cell_focus_out),
                     this);

    // Connect keyboard navigation
    g_signal_connect(entry, "key-press-event", G_CALLBACK(on_cell_key_press),
                     this);

    // Add to grid (column offset by 3 for fixed columns)
    gtk_grid_attach(GTK_GRID(grid_), entry, step_idx + 3, row, 1, 1);
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_cell_values(const Controller_state& state) {
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

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_playhead_highlighting(const Controller_state& state) {
  // Clear old playhead highlighting
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context = gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "playhead");
  }

  // Add new playhead highlighting
  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];

    // Skip sequences with no steps to avoid division by zero
    if (seq.num_steps == 0) {
      continue;
    }

    std::size_t playing_step = (seq.current_step_idx - 2) % seq.num_steps;
    std::cout << "[DEBUG] CUR PLAYING STEP: " << playing_step << std::endl;

    // Highlight all parameters for the current step of this sequence
    for (std::size_t param_idx = 0; param_idx < seq.num_params; ++param_idx) {
      std::string key = make_cell_key(seq_idx, playing_step, param_idx);
      std::cout << "[DEBUG] cell key: " << key << std::endl;
      auto it = cell_widgets_.find(key);
      if (it != cell_widgets_.end()) {
        GtkStyleContext* context =
            gtk_widget_get_style_context(it->second.entry);
        gtk_style_context_add_class(context, "playhead");
      }
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_selection_highlighting(
    const Controller_state& state) {
  // Clear old selection highlighting
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context = gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "selected");
  }

  // Add new selection highlighting
  if (state.selected_seq && state.selected_event && state.selected_param_idx) {
    std::string key = make_cell_key(*state.selected_seq, *state.selected_event,
                                    *state.selected_param_idx);
    auto it = cell_widgets_.find(key);
    if (it != cell_widgets_.end()) {
      GtkStyleContext* context = gtk_widget_get_style_context(it->second.entry);
      gtk_style_context_add_class(context, "selected");
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::update_play_icons(const Controller_state& state) {
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

template <sequencable::Mut_seq_event Event_t>
std::string Gui<Event_t>::make_cell_key(std::size_t seq, std::size_t step,
                                        std::size_t param) const {
  std::ostringstream ss;
  ss << seq << "_" << step << "_" << param;
  return ss.str();
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::clear_highlighting() {
  for (auto& [key, cell] : cell_widgets_) {
    GtkStyleContext* context = gtk_widget_get_style_context(cell.entry);
    gtk_style_context_remove_class(context, "playhead");
    gtk_style_context_remove_class(context, "selected");
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::scroll_to_selection(const Controller_state& state) {
  if (!state.selected_seq || !state.selected_event ||
      !state.selected_param_idx) {
    return;
  }

  std::string key = make_cell_key(*state.selected_seq, *state.selected_event,
                                  *state.selected_param_idx);
  auto it = cell_widgets_.find(key);
  if (it != cell_widgets_.end()) {
    // Scroll to make the selected cell visible
    GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(
        GTK_SCROLLED_WINDOW(scrolled_window_));
    GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(
        GTK_SCROLLED_WINDOW(scrolled_window_));

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

// Helper to commit cell edit
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::commit_cell_edit(Gui<Event_t>* gui, GtkEntry* entry) {
  const char* key =
      static_cast<const char*>(g_object_get_data(G_OBJECT(entry), "cell_key"));

  if (key) {
    auto it = gui->cell_widgets_.find(key);
    if (it != gui->cell_widgets_.end()) {
      const char* new_value_str = gtk_entry_get_text(entry);

      // Check if this is a duration cell (param_idx == SIZE_MAX)
      if (it->second.param_idx == SIZE_MAX) {
        // Duration cell - parse as double
        double duration_value;

        // Handle empty string as default (0.25)
        if (new_value_str == nullptr || std::string(new_value_str).empty()) {
          duration_value = 0.25;
        } else {
          std::istringstream iss(new_value_str);
          if (!(iss >> duration_value)) {
            std::cerr << "[ERROR] Failed to parse duration value: "
                      << new_value_str << std::endl;
            return;
          }
        }

        std::cout << "[INFO] Duration edited: seq=" << it->second.seq_idx
                  << " step=" << it->second.step_idx
                  << " duration=" << duration_value << std::endl;

        // Update the duration in controller
        try {
          gui->controller_->update_step_duration(
              it->second.seq_idx, it->second.step_idx, duration_value);
        } catch (const std::exception& e) {
          std::cerr << "[ERROR] Failed to update duration: " << e.what()
                    << std::endl;
        }
      } else {
        // Parameter cell - parse as Event_t
        Event_t new_value;

        // Handle empty string as 0
        if (new_value_str == nullptr || std::string(new_value_str).empty()) {
          new_value = Event_t{0};
        } else {
          std::istringstream iss(new_value_str);
          if (!(iss >> new_value)) {
            std::cerr << "[ERROR] Failed to parse value: " << new_value_str
                      << std::endl;
            return;
          }
        }

        std::cout << "[INFO] Cell edited: seq=" << it->second.seq_idx
                  << " step=" << it->second.step_idx
                  << " param=" << it->second.param_idx << " value=" << new_value
                  << std::endl;

        // Update the controller
        try {
          gui->controller_->update(it->second.seq_idx, it->second.step_idx,
                                   it->second.param_idx, std::move(new_value));
        } catch (const std::exception& e) {
          std::cerr << "[ERROR] Failed to update controller: " << e.what()
                    << std::endl;
        }
      }
    }
  }
}

// Static event handlers
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_cell_edited(GtkEntry* entry, gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);
  commit_cell_edit(gui, entry);
}

template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_cell_focus_out(GtkWidget* widget,
                                         GdkEventFocus* event,
                                         gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);
  GtkEntry* entry = GTK_ENTRY(widget);
  commit_cell_edit(gui, entry);
  return FALSE; // Allow default handling
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::on_expand_clicked(GtkButton* button, gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);
  std::size_t seq_idx =
      GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(button), "seq_idx"));

  // Toggle expansion state
  gui->expanded_seqs_[seq_idx] = !gui->expanded_seqs_[seq_idx];

  std::cout << "[INFO] Toggled expansion for seq " << seq_idx << " to "
            << (gui->expanded_seqs_[seq_idx] ? "expanded" : "collapsed")
            << std::endl;

  // Trigger a full grid rebuild
  auto state = gui->controller_->get_state();
  gui->rebuild_grid(state);
  gui->last_state_ = state;
}

template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_cell_key_press(GtkWidget* widget, GdkEventKey* event,
                                         gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);
  const char* key =
      static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "cell_key"));

  if (!key) {
    return FALSE; // Not handled
  }

  auto it = gui->cell_widgets_.find(key);
  if (it == gui->cell_widgets_.end()) {
    return FALSE;
  }

  const CellWidget& cell = it->second;
  bool handled = false;

  // Handle navigation keys
  switch (event->keyval) {
  case GDK_KEY_Up:
    // Commit edits before navigating
    commit_cell_edit(gui, GTK_ENTRY(widget));
    // Move to previous parameter
    gui->controller_->select(cell.seq_idx, cell.step_idx);
    if (cell.param_idx == SIZE_MAX) {
      // Duration cell - move to last parameter
      gui->controller_->select_prev_param();
      handled = true;
    } else if (cell.param_idx > 0) {
      // Regular parameter cell - move up
      gui->controller_->select_param(cell.param_idx - 1);
      handled = true;
    }
    break;

  case GDK_KEY_Down:
    // Commit edits before navigating
    commit_cell_edit(gui, GTK_ENTRY(widget));
    // Move to next parameter
    gui->controller_->select(cell.seq_idx, cell.step_idx);
    // If we're on a duration cell, wrap to param 0
    if (cell.param_idx == SIZE_MAX) {
      gui->controller_->select_param(0);
    } else {
      gui->controller_->select_param(cell.param_idx);
      gui->controller_->select_next_param();
    }
    handled = true;
    break;

  case GDK_KEY_Left:
    // Commit edits before navigating
    commit_cell_edit(gui, GTK_ENTRY(widget));
    // Move to previous step (wrap around)
    gui->controller_->select(cell.seq_idx, cell.step_idx);
    // Use param 0 if we're on a duration cell (SIZE_MAX)
    gui->controller_->select_param(cell.param_idx == SIZE_MAX ? 0
                                                              : cell.param_idx);
    gui->controller_->select_prev_step();
    handled = true;
    break;

  case GDK_KEY_Right:
    // Commit edits before navigating
    commit_cell_edit(gui, GTK_ENTRY(widget));
    // Move to next step
    gui->controller_->select(cell.seq_idx, cell.step_idx);
    // Use param 0 if we're on a duration cell (SIZE_MAX)
    gui->controller_->select_param(cell.param_idx == SIZE_MAX ? 0
                                                              : cell.param_idx);
    gui->controller_->select_next_step();
    handled = true;
    break;

  case GDK_KEY_Tab:
  case GDK_KEY_ISO_Left_Tab: {
    // Commit any edits before navigating
    commit_cell_edit(gui, GTK_ENTRY(widget));

    // Tab: move right, Shift+Tab: move left
    bool shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;

    // Update selection in controller
    gui->controller_->select(cell.seq_idx, cell.step_idx);
    // Use param 0 if we're on a duration cell (SIZE_MAX)
    gui->controller_->select_param(cell.param_idx == SIZE_MAX ? 0
                                                              : cell.param_idx);

    if (shift_pressed) {
      // Shift+Tab: move to previous step
      gui->controller_->select_prev_step();
    } else {
      // Tab: move to next step
      gui->controller_->select_next_step();
    }

    // Get the new state and find the new cell
    auto state = gui->controller_->get_state();
    if (state.selected_seq && state.selected_event &&
        state.selected_param_idx) {
      std::string new_key =
          gui->make_cell_key(*state.selected_seq, *state.selected_event,
                             *state.selected_param_idx);
      auto new_cell_it = gui->cell_widgets_.find(new_key);
      if (new_cell_it != gui->cell_widgets_.end()) {
        // Give focus to the new cell
        gtk_widget_grab_focus(new_cell_it->second.entry);
      }
    }

    // Update highlighting
    gui->update_selection_highlighting(state);
    gui->scroll_to_selection(state);
    gui->last_state_ = state;

    handled = true;
    break;
  }

  case GDK_KEY_Page_Up:
    // Move to previous sequence
    gui->controller_->select_prev_seq();
    // Use param 0 if we're on a duration cell (SIZE_MAX)
    gui->controller_->select_param(cell.param_idx == SIZE_MAX ? 0
                                                              : cell.param_idx);
    handled = true;
    break;

  case GDK_KEY_Page_Down:
    // Move to next sequence
    gui->controller_->select_next_seq();
    // Use param 0 if we're on a duration cell (SIZE_MAX)
    gui->controller_->select_param(cell.param_idx == SIZE_MAX ? 0
                                                              : cell.param_idx);
    handled = true;
    break;

  case GDK_KEY_Escape: {
    // Cancel edit and revert to original value
    auto state = gui->controller_->get_state();

    // Get original value from controller
    if (cell.seq_idx < state.sequencers.size()) {
      const auto& controller = state.sequencers[cell.seq_idx];
      if (cell.step_idx < controller.steps.size()) {
        const auto& step_state = controller.steps[cell.step_idx];

        if (cell.param_idx == SIZE_MAX) {
          // Duration cell - revert to original duration
          std::ostringstream oss;
          oss << step_state.duration_seconds;
          std::string duration_str = oss.str();
          // Remove trailing zeros
          if (duration_str.find('.') != std::string::npos) {
            duration_str.erase(duration_str.find_last_not_of('0') + 1,
                               std::string::npos);
            if (!duration_str.empty() && duration_str.back() == '.') {
              duration_str.pop_back();
            }
          }
          gtk_entry_set_text(GTK_ENTRY(widget), duration_str.c_str());
        } else if (cell.param_idx < step_state.param_values.size()) {
          // Parameter cell - revert to original parameter value
          const std::string& original_value =
              step_state.param_values[cell.param_idx];
          gtk_entry_set_text(GTK_ENTRY(widget), original_value.c_str());
        }
      }
    }

    // Remove focus from the cell
    if (gui->window_) {
      gtk_widget_grab_focus(gui->window_);
    }

    handled = true;
    break;
  }

  default:
    break;
  }

  if (handled) {
    // Update GUI to reflect new selection
    auto state = gui->controller_->get_state();

    // For non-Tab navigation, also grab focus on the new cell
    if (event->keyval != GDK_KEY_Tab && event->keyval != GDK_KEY_ISO_Left_Tab) {
      if (state.selected_seq && state.selected_event &&
          state.selected_param_idx) {
        std::string new_key =
            gui->make_cell_key(*state.selected_seq, *state.selected_event,
                               *state.selected_param_idx);
        auto new_cell_it = gui->cell_widgets_.find(new_key);
        if (new_cell_it != gui->cell_widgets_.end()) {
          gtk_widget_grab_focus(new_cell_it->second.entry);
        }
      }
    }

    gui->update_selection_highlighting(state);
    gui->scroll_to_selection(state);
    gui->last_state_ = state;
    return TRUE; // Event handled
  }

  return FALSE; // Event not handled, allow default processing
}

// Step operations

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::toggle_selected_step() {
  auto state = controller_->get_state();
  if (state.selected_seq && state.selected_event) {
    try {
      controller_->toggle_step(*state.selected_seq, *state.selected_event);
      std::cout << "[INFO] Toggled step " << *state.selected_event
                << " in sequence " << *state.selected_seq << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to toggle step: " << e.what() << std::endl;
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::add_step_to_selected() {
  auto state = controller_->get_state();
  if (state.selected_seq) {
    try {
      controller_->add_step(*state.selected_seq);
      std::cout << "[INFO] Added step to sequence " << *state.selected_seq
                << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to add step: " << e.what() << std::endl;
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::insert_step_before_selected() {
  auto state = controller_->get_state();
  if (state.selected_seq && state.selected_event) {
    try {
      controller_->insert_step(*state.selected_seq, *state.selected_event);
      std::cout << "[INFO] Inserted step at " << *state.selected_event
                << " in sequence " << *state.selected_seq << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to insert step: " << e.what() << std::endl;
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::remove_selected_step() {
  auto state = controller_->get_state();
  if (state.selected_seq && state.selected_event) {
    try {
      controller_->remove_step(*state.selected_seq, *state.selected_event);
      std::cout << "[INFO] Removed step " << *state.selected_event
                << " from sequence " << *state.selected_seq << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to remove step: " << e.what() << std::endl;
    }
  }
}

// Sequence operations

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::add_new_sequence() {
  try {
    // Create a sequence with 8 steps and 3 parameters by default
    controller_->add_sequence(8, 3);
    std::cout << "[INFO] Added new sequence" << std::endl;

    // Select the new sequence if it has steps
    std::size_t new_seq_idx = controller_->size() - 1;
    if (controller_->size() > 0) {
      auto& new_seq = controller_->operator[](new_seq_idx);
      if (new_seq.size() > 0) {
        controller_->select(new_seq_idx, 0);
        if (controller_->size() > 0) {
          auto first_step = new_seq.at(0);
          if (first_step.params.size() > 0) {
            controller_->select_param(0);
          }
        }
      }
      expanded_seqs_[new_seq_idx] = true;
    }
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Failed to add sequence: " << e.what() << std::endl;
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::remove_selected_sequence() {
  auto state = controller_->get_state();
  if (state.selected_seq) {
    try {
      controller_->remove_sequence(*state.selected_seq);
      std::cout << "[INFO] Removed sequence " << *state.selected_seq
                << std::endl;

      // Clean up expanded state
      expanded_seqs_.erase(*state.selected_seq);
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to remove sequence: " << e.what()
                << std::endl;
    }
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::duplicate_selected_sequence() {
  auto state = controller_->get_state();
  if (state.selected_seq) {
    try {
      controller_->duplicate_sequence(*state.selected_seq);
      std::cout << "[INFO] Duplicated sequence " << *state.selected_seq
                << std::endl;

      // Expand the duplicate
      expanded_seqs_[controller_->size() - 1] = true;
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Failed to duplicate sequence: " << e.what()
                << std::endl;
    }
  }
}

// Expand/collapse operations

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::expand_all_sequences() {
  for (std::size_t i = 0; i < controller_->size(); ++i) {
    expanded_seqs_[i] = true;
  }
  auto state = controller_->get_state();
  rebuild_grid(state);
  last_state_ = state;
  std::cout << "[INFO] Expanded all sequences" << std::endl;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::collapse_all_sequences() {
  for (std::size_t i = 0; i < controller_->size(); ++i) {
    expanded_seqs_[i] = false;
  }
  auto state = controller_->get_state();
  rebuild_grid(state);
  last_state_ = state;
  std::cout << "[INFO] Collapsed all sequences" << std::endl;
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::toggle_selected_sequence_expand() {
  auto state = controller_->get_state();
  if (state.selected_seq) {
    std::size_t seq_idx = *state.selected_seq;
    expanded_seqs_[seq_idx] = !expanded_seqs_[seq_idx];

    auto new_state = controller_->get_state();
    rebuild_grid(new_state);
    last_state_ = new_state;

    std::cout << "[INFO] Toggled expansion for sequence " << seq_idx
              << std::endl;
  }
}

// File operations

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::load_from_json(const std::string& filename) {
  try {
    controller_->load_from_json(filename);

    // Expand first sequence
    expanded_seqs_.clear();
    if (controller_->size() > 0) {
      expanded_seqs_[0] = true;
      controller_->select(0, 0);
    }

    // Rebuild GUI
    auto state = controller_->get_state();
    rebuild_grid(state);
    last_state_ = state;

    // Show success dialog
    GtkWidget* dialog =
        gtk_message_dialog_new(GTK_WINDOW(window_), GTK_DIALOG_MODAL,
                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "File Loaded");

    std::ostringstream msg;
    msg << "Loaded " << controller_->size() << " sequences from:\n" << filename;
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             msg.str().c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    std::cout << "[INFO] Loaded session from: " << filename << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Failed to load file: " << e.what() << std::endl;

    // Show error dialog
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(window_), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "Load Failed");

    std::ostringstream msg;
    msg << "Failed to load from:\n" << filename << "\n\nError: " << e.what();
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             msg.str().c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::save_to_json(const std::string& filename) {
  // Get current display state
  auto state = controller_->get_state();

  // Build JSON manually (avoiding external dependencies)
  std::ostringstream json;
  json << "{\n  \"sequences\": [\n";

  for (std::size_t seq_idx = 0; seq_idx < state.sequencers.size(); ++seq_idx) {
    const auto& seq = state.sequencers[seq_idx];

    if (seq_idx > 0) {
      json << ",\n";
    }
    json << "    [\n";

    for (std::size_t step_idx = 0; step_idx < seq.steps.size(); ++step_idx) {
      const auto& step = seq.steps[step_idx];

      if (step_idx > 0) {
        json << ",\n";
      }
      json << "      [";

      for (std::size_t param_idx = 0; param_idx < step.param_values.size();
           ++param_idx) {
        if (param_idx > 0) {
          json << ", ";
        }
        // Output parameter value (already a string, convert to number if
        // possible)
        const std::string& val_str = step.param_values[param_idx];
        if (val_str.empty()) {
          json << "0";
        } else {
          json << val_str;
        }
      }

      json << "]";
    }

    json << "\n    ]";
  }

  json << "\n  ]\n}\n";

  // Write to file
  std::ofstream file(filename);
  if (file.is_open()) {
    file << json.str();
    file.close();
    std::cout << "[INFO] Saved sequences to: " << filename << std::endl;

    // Show success dialog
    GtkWidget* dialog =
        gtk_message_dialog_new(GTK_WINDOW(window_), GTK_DIALOG_MODAL,
                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "File Saved");

    std::ostringstream msg;
    msg << "Sequences saved to:\n" << filename;
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             msg.str().c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  } else {
    std::cerr << "[ERROR] Failed to open file for writing: " << filename
              << std::endl;

    // Show error dialog
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(window_), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "Save Failed");

    std::ostringstream msg;
    msg << "Failed to save to:\n" << filename;
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             msg.str().c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

template <sequencable::Mut_seq_event Event_t> void Gui<Event_t>::edit_bpm() {
  // Create dialog with entry field for BPM
  GtkWidget* dialog = gtk_dialog_new_with_buttons(
      "Edit BPM", GTK_WINDOW(window_), GTK_DIALOG_MODAL, "_OK", GTK_RESPONSE_OK,
      "_Cancel", GTK_RESPONSE_CANCEL, NULL);

  GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  // Add label
  GtkWidget* label = gtk_label_new("Enter BPM (beats per minute):");
  gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 5);

  // Add entry for BPM
  GtkWidget* entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), "120"); // Default BPM
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);
  gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 5);

  gtk_widget_show_all(dialog);

  // Run dialog
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    const char* bpm_str = gtk_entry_get_text(GTK_ENTRY(entry));
    try {
      double bpm = std::stod(bpm_str);
      if (bpm > 0 && bpm <= 999) {
        controller_->set_all_durations_from_bpm(bpm);

        // Refresh display
        auto state = controller_->get_state();
        rebuild_grid(state);
        last_state_ = state;

        std::cout << "[INFO] Set BPM to " << bpm << std::endl;
      } else {
        std::cerr << "[ERROR] BPM must be between 1 and 999" << std::endl;
      }
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Invalid BPM value: " << bpm_str << std::endl;
    }
  }

  gtk_widget_destroy(dialog);
}

template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::show_help_dialog() {
  GtkWidget* dialog = gtk_message_dialog_new(
      GTK_WINDOW(window_), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
      "Keyboard Shortcuts");

  const char* help_text = "Navigation:\n"
                          "  ↑/↓           - Move between parameters\n"
                          "  ←/→           - Move between steps\n"
                          "  Tab           - Move to next step (right)\n"
                          "  Shift+Tab     - Move to previous step (left)\n"
                          "  Page Up/Down  - Move between sequences\n"
                          "\n"
                          "Transport:\n"
                          "  Space         - Start/stop all sequences\n"
                          "  Ctrl+Space    - Start/stop selected sequence\n"
                          "\n"
                          "Editing:\n"
                          "  Type digits   - Start editing selected cell\n"
                          "  Enter         - Commit edit and stay\n"
                          "  T             - Toggle step on/off\n"
                          "\n"
                          "Steps:\n"
                          "  Ctrl+A        - Add step to sequence\n"
                          "  Ctrl+I        - Insert step before selected\n"
                          "  Ctrl+D        - Remove selected step\n"
                          "\n"
                          "Sequences:\n"
                          "  Ctrl+Shift+A  - Add new sequence\n"
                          "  Ctrl+Shift+D  - Remove selected sequence\n"
                          "  Ctrl+Shift+C  - Duplicate selected sequence\n"
                          "\n"
                          "View:\n"
                          "  Ctrl+E        - Expand all sequences\n"
                          "  Ctrl+W        - Collapse all sequences\n"
                          "  Ctrl+T        - Toggle selected sequence\n"
                          "  ▶/▼ button    - Expand/collapse sequence\n"
                          "\n"
                          "File:\n"
                          "  Ctrl+S        - Save to JSON file\n"
                          "  Ctrl+L        - Load from JSON file\n"
                          "  F10           - Show menu\n"
                          "  F1            - Show this help";

  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           help_text);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

template <sequencable::Mut_seq_event Event_t>
gboolean Gui<Event_t>::on_window_key_press(GtkWidget* widget,
                                           GdkEventKey* event,
                                           gpointer user_data) {
  Gui* gui = static_cast<Gui*>(user_data);

  // Check for F1 help
  if (event->keyval == GDK_KEY_F1) {
    gui->show_help_dialog();
    return TRUE; // Event handled
  }

  // Check for Ctrl+S to save
  if (event->keyval == GDK_KEY_s || event->keyval == GDK_KEY_S) {
    bool ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;

    if (ctrl_pressed) {
      // Generate default filename with timestamp
      auto now = std::time(nullptr);
      auto tm = *std::localtime(&now);
      std::ostringstream filename;
      filename << "sequences_" << (tm.tm_year + 1900) << std::setfill('0')
               << std::setw(2) << (tm.tm_mon + 1) << std::setw(2) << tm.tm_mday
               << "_" << std::setw(2) << tm.tm_hour << std::setw(2) << tm.tm_min
               << std::setw(2) << tm.tm_sec << ".json";

      gui->save_to_json(filename.str());
      return TRUE; // Event handled
    }
  }

  // Check for Space or Ctrl+Space
  if (event->keyval == GDK_KEY_space) {
    bool ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;

    if (ctrl_pressed) {
      // Ctrl+Space: Toggle selected sequence only
      auto state = gui->controller_->get_state();
      if (state.selected_seq) {
        std::size_t seq_idx = *state.selected_seq;

        if (gui->controller_->is_running(seq_idx)) {
          std::cout << "[INFO] Stopping sequence " << seq_idx << std::endl;
          gui->controller_->stop(seq_idx);
        } else {
          std::cout << "[INFO] Starting sequence " << seq_idx << std::endl;
          gui->controller_->start(seq_idx);
        }

        return TRUE; // Event handled
      }
    } else {
      // Space: Toggle all sequences
      if (gui->controller_->any_running()) {
        std::cout << "[INFO] Stopping all sequences" << std::endl;
        gui->controller_->stop_all();
      } else {
        std::cout << "[INFO] Starting all sequences" << std::endl;
        gui->controller_->start_all();
      }

      return TRUE; // Event handled
    }
  }

  // Check for modifier keys
  bool ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;
  bool shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;

  // F10 to show menu (handled by GTK automatically, but we can add custom
  // handling if needed)
  if (event->keyval == GDK_KEY_F10) {
    return FALSE; // Let GTK handle menu activation
  }

  // Toggle step (T)
  if (event->keyval == GDK_KEY_t || event->keyval == GDK_KEY_T) {
    if (ctrl_pressed && !shift_pressed) {
      // Ctrl+T: Toggle selected sequence expand/collapse
      gui->toggle_selected_sequence_expand();
      return TRUE;
    } else if (!ctrl_pressed && !shift_pressed) {
      // T: Toggle step
      gui->toggle_selected_step();
      return TRUE;
    }
  }

  // Add operations
  if (event->keyval == GDK_KEY_a || event->keyval == GDK_KEY_A) {
    if (ctrl_pressed && shift_pressed) {
      // Ctrl+Shift+A: Add sequence
      gui->add_new_sequence();
      return TRUE;
    } else if (ctrl_pressed) {
      // Ctrl+A: Add step
      gui->add_step_to_selected();
      return TRUE;
    }
  }

  // Insert step (Ctrl+I)
  if ((event->keyval == GDK_KEY_i || event->keyval == GDK_KEY_I) &&
      ctrl_pressed && !shift_pressed) {
    gui->insert_step_before_selected();
    return TRUE;
  }

  // Remove/Delete operations
  if (event->keyval == GDK_KEY_d || event->keyval == GDK_KEY_D) {
    if (ctrl_pressed && shift_pressed) {
      // Ctrl+Shift+D: Remove sequence
      gui->remove_selected_sequence();
      return TRUE;
    } else if (ctrl_pressed) {
      // Ctrl+D: Remove step
      gui->remove_selected_step();
      return TRUE;
    }
  }

  // Duplicate sequence (Ctrl+Shift+C)
  if ((event->keyval == GDK_KEY_c || event->keyval == GDK_KEY_C) &&
      ctrl_pressed && shift_pressed) {
    gui->duplicate_selected_sequence();
    return TRUE;
  }

  // Expand all (Ctrl+E)
  if ((event->keyval == GDK_KEY_e || event->keyval == GDK_KEY_E) &&
      ctrl_pressed && !shift_pressed) {
    gui->expand_all_sequences();
    return TRUE;
  }

  // Collapse all (Ctrl+W)
  if ((event->keyval == GDK_KEY_w || event->keyval == GDK_KEY_W) &&
      ctrl_pressed && !shift_pressed) {
    gui->collapse_all_sequences();
    return TRUE;
  }

  // Edit BPM (Ctrl+B)
  if ((event->keyval == GDK_KEY_b || event->keyval == GDK_KEY_B) &&
      ctrl_pressed && !shift_pressed) {
    gui->edit_bpm();
    return TRUE;
  }

  // Load file (Ctrl+L)
  if ((event->keyval == GDK_KEY_l || event->keyval == GDK_KEY_L) &&
      ctrl_pressed && !shift_pressed) {
    // Show file chooser dialog
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Load Session", GTK_WINDOW(gui->window_), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      gui->load_from_json(filename);
      g_free(filename);
    }

    gtk_widget_destroy(dialog);
    return TRUE;
  }

  return FALSE; // Event not handled
}

} // namespace gui

} // namespace Micro_composer
