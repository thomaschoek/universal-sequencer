#ifndef MICRO_COMPOSER_GUI_H
#define MICRO_COMPOSER_GUI_H

#include "controller/poly_sequencer_controller.h"
#include "sequencable/concepts.h"
#include <gtk/gtk.h>

namespace Micro_composer {

namespace gui {

template <sequencable::Mut_seq_event Event_t> struct Gui {
  // Provides a graphical user interface to numerous sequencers controlled by a
  // Poly_sequencer_controller
  using Controller = controller::Poly_sequencer_controller<Event_t>;
  using Controller_state = Controller::State;
  using Sequencer = Controller::Sequencer_t;

private:
  // Maybe the event loop should be static so that it can run on its own thread?
  // Or maybe not?
  static void event_loop(Gui& gui, Controller& controller,
                         unsigned int fps = 50);
};

} // namespace gui

} // namespace Micro_composer

#include "gui/gui.tpp"

#endif // MICRO_COMPOSER_GUI_H
