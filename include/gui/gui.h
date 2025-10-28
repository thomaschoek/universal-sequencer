#ifndef MICRO_COMPOSER_GUI_H
#define MICRO_COMPOSER_GUI_H

#include "controller/poly_sequencer_controller.h"
#include "sequencable/concepts.h"
#include <gtk/gtk.h>
#include <map>
#include <memory>
#include <string>

namespace Micro_composer {

namespace gui {

template <sequencable::Mut_seq_event Event_t> struct Gui {
  using Controller = controller::Poly_sequencer_controller<Event_t>;
  using Controller_state = Controller::State;
  using Sequencer = Controller::Sequencer_t;

  explicit Gui(std::shared_ptr<Controller> controller);

private:
  void check_for_input();
  void handle_input(Controller& controller);
  static void event_loop(Gui& gui, Controller& controller,
                         unsigned int fps = 50);
  std::atomic<bool> invalidated_{true};
  Controller& controller_;
};

} // namespace gui

} // namespace Micro_composer

#include "gui/gui.tpp"

#endif // MICRO_COMPOSER_GUI_H
