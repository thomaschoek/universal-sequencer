#ifndef MICRO_COMPOSER_GUI_H
#define MICRO_COMPOSER_GUI_H

#include "gui/display_state.h"
#include <gtk/gtk.h>
#include <map>
#include <string>

namespace Micro_composer {

namespace gui {

class Gui {
public:
  Gui();
  ~Gui();

  // Render the current state to the display
  void render(const Display_state& state);

  // Initialize GTK and create window
  void init(int argc, char** argv);

  // Show the window
  void show();

private:
  // GTK widgets
  GtkWidget* window_{nullptr};
  GtkWidget* main_box_{nullptr};
  GtkWidget* scrolled_window_{nullptr};
  GtkWidget* grid_{nullptr}; // Main grid containing all cells

  // Last rendered state for comparison
  Display_state last_state_;

  // Widget tracking for updates
  struct CellWidget {
    GtkWidget* entry;
    std::size_t seq_idx;
    std::size_t step_idx;
    std::size_t param_idx;
  };
  std::map<std::string, CellWidget> cell_widgets_; // Key: "seq_step_param"

  struct SeqHeaderWidgets {
    GtkWidget* expand_button;
    GtkWidget* play_icon;
    GtkWidget* name_label;
  };
  std::map<std::size_t, SeqHeaderWidgets> seq_headers_;

  // Helper methods for building UI
  void rebuild_grid(const Display_state& state);
  void create_sequence_header(std::size_t seq_idx,
                               const Sequencer_display_state& seq_state,
                               int row);
  void create_parameter_row(std::size_t seq_idx, std::size_t param_idx,
                            const Sequencer_display_state& seq_state, int row);

  // Helper methods for rendering updates
  void update_cell_values(const Display_state& state);
  void update_playhead_highlighting(const Display_state& state);
  void update_selection_highlighting(const Display_state& state);
  void update_play_icons(const Display_state& state);

  // Helper methods for cell management
  std::string make_cell_key(std::size_t seq, std::size_t step,
                           std::size_t param) const;
  void clear_highlighting();
  void scroll_to_selection(const Display_state& state);

  // Event handlers
  static void on_cell_edited(GtkEntry* entry, gpointer user_data);
  static void on_expand_clicked(GtkButton* button, gpointer user_data);
};

} // namespace gui

} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_H
