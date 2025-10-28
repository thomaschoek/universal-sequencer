#ifndef MICRO_COMPOSER_GUI_H
#define MICRO_COMPOSER_GUI_H

#include "controller/poly_sequencer_controller.h"
#include "sequencable/concepts.h"
#include <chrono>
#include <functional>
#include <gtk/gtk.h>
#include <unordered_map>

namespace Micro_composer {

namespace gui {

template <sequencable::Mut_seq_event Event_t> class Gui {
public:
  using Controller = controller::Poly_sequencer_controller<Event_t>;
  using Controller_state = Controller::State;
  using Sequencer = Controller::Sequencer_t;
  using Seq_idx = Controller::Seq_idx;
  using Event_idx = Controller::Event_idx;
  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::duration<double>;

  // GUI operation modes (vim-like)
  enum class Mode { Normal, Edit };

  // GUI-specific state
  struct Gui_state {
    // Reference to controller state (controller owns it)
    Controller_state controller_state;

    // Per-sequencer GUI state
    struct Per_sequencer_gui_state {
      Event_idx last_rendered_playhead{0};
      bool playhead_visible{false};
    };
    std::vector<Per_sequencer_gui_state> sequencer_gui_states;

    // Global GUI state
    Mode mode{Mode::Normal};
    bool state_dirty{true}; // True on first render
  };

  // Constructor
  explicit Gui(Controller& controller, unsigned int fps = 50);

  // Destructor
  ~Gui();

  // Run the GUI event loop
  void run();

  // GUI wrappers for controller actions (update GUI state and mark dirty)
  void gui_select(Seq_idx seq_idx, Event_idx event_idx);
  void gui_select_next_seq();
  void gui_select_prev_seq();
  void gui_select_next_pos();
  void gui_select_prev_pos();
  void gui_start(Seq_idx seq_idx);
  void gui_pause(Seq_idx seq_idx);
  void gui_stop(Seq_idx seq_idx);
  void gui_toggle_play(Seq_idx seq_idx);

private:
  // Controller actions (mapped to keyboard events)
  using Controller_action = std::function<void()>;

  // Event loop components
  void process_input_events();
  void update_playheads();
  void render();

  // GTK callbacks
  static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event,
                                gpointer user_data);
  static gboolean on_tick(gpointer user_data);

  // Keyboard event handlers
  void handle_normal_mode_key(guint keyval);
  void handle_edit_mode_key(guint keyval);

  // Initialize GTK widgets
  void init_widgets();
  void build_grid();

  // Rendering helpers
  void render_grid();
  void update_cell(Seq_idx seq_idx, Event_idx event_idx);
  void update_playhead_visual(Seq_idx seq_idx, Event_idx old_pos,
                              Event_idx new_pos);
  void update_window_title();
  void focus_selected_cell();

  // Data members
  Controller& controller_;
  Gui_state state_;
  unsigned int fps_;
  Duration frame_duration_;
  bool running_{false};

  // GTK widgets
  GtkWidget* window_{nullptr};
  GtkWidget* grid_{nullptr};
  std::vector<std::vector<GtkWidget*>> cell_entries_;

  // Keyboard event mapping
  std::unordered_map<guint, Controller_action> normal_mode_actions_;
  std::unordered_map<guint, Controller_action> edit_mode_actions_;
};

} // namespace gui

} // namespace Micro_composer

#include "gui/gui.tpp"

#endif // MICRO_COMPOSER_GUI_H
