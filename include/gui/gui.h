#ifndef MICRO_COMPOSER_GUI_H
#define MICRO_COMPOSER_GUI_H

#include "gui/display_state.h"
#include <gtk/gtk.h>

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
  GtkWidget* grid_{nullptr}; // Table of steps

  // Last rendered state for comparison
  Display_state last_state_;

  // Helper methods for rendering
  void render_sequencer_row(std::size_t seq_idx,
                            const Sequencer_display_state& seq_state);
  void render_selection(const Display_state& state);
};

} // namespace gui

} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_H
