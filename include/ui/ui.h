#ifndef MICRO_COMPOSER_USER_INTERFACE_H
#define MICRO_COMPOSER_USER_INTERFACE_H

#include "controller/matrix_sequencer_controller.h"
#include "gui/display_state.h"
#include "gui/gui.h"
#include <memory>

namespace Micro_composer {

namespace user_interface {

template <typename T_event_params>
class User_interface {
public:
  using Controller = controller::Matrix_sequencer_controller<T_event_params>;

  // Constructor
  explicit User_interface(std::shared_ptr<Controller> controller);

  // Initialize the GUI system (GTK, etc.)
  void init();

  // Start the event loop
  void run();

  // Stop the event loop
  void stop();

  // Notify the GUI about state changes (called by timer)
  void notify();

private:
  // Update callback - polls controller and updates GUI if state changed
  bool on_update_timer();

  std::shared_ptr<Controller> controller_;
  std::unique_ptr<gui::Gui> gui_;

  gui::Display_state last_display_state_;

  // Timer ID for periodic updates (GTK timer)
  unsigned int timer_id_{0};
  bool running_{false};
};

} // namespace user_interface

} // namespace Micro_composer

#include "ui/ui.tpp"

#endif // MICRO_COMPOSER_USER_INTERFACE_H
