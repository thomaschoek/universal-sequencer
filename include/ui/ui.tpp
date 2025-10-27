#include "ui/ui.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <iostream>

namespace Micro_composer {

namespace user_interface {

template <sequencable::Mut_seq_event T_event_params>
User_interface<T_event_params>::User_interface(
    std::shared_ptr<Controller> controller)
    : controller_(controller),
      gui_(std::make_unique<gui::Gui<T_event_params>>(controller)) {}

template <sequencable::Mut_seq_event T_event_params>
void User_interface<T_event_params>::init(int argc, char** argv) {
  // Initialize GUI (GTK)
  gui_->init(argc, argv);
  gui_->show();
  running_ = false;
}

template <sequencable::Mut_seq_event T_event_params>
void User_interface<T_event_params>::run() {
  if (running_) {
    return;
  }

  running_ = true;

  // Set up a timer to poll the controller state at ~30 FPS (33ms interval)
  timer_id_ = g_timeout_add(
      33, // milliseconds
      [](gpointer user_data) -> gboolean {
        auto* self = static_cast<User_interface*>(user_data);
        return self->on_update_timer() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
      },
      this);

  std::cout << "[INFO] User interface event loop started (30 FPS polling)"
            << std::endl;

  // Start GTK main loop
  gtk_main();
}

template <sequencable::Mut_seq_event T_event_params>
void User_interface<T_event_params>::stop() {
  if (!running_) {
    return;
  }

  running_ = false;

  if (timer_id_ != 0) {
    g_source_remove(timer_id_);
    timer_id_ = 0;
  }

  std::cout << "[INFO] User interface event loop stopped" << std::endl;
}

template <sequencable::Mut_seq_event T_event_params>
void User_interface<T_event_params>::notify() {
  if (!controller_ || !gui_) {
    return;
  }

  // Get current state from controller (thread-safe atomic read)
  auto current_state = controller_->get_state();

  // Only update GUI if state has actually changed
  if (current_state != last_controller_state_) {
    gui_->render(current_state);
    last_controller_state_ = current_state;
  }
}

template <sequencable::Mut_seq_event T_event_params>
bool User_interface<T_event_params>::on_update_timer() {
  if (!running_) {
    return false; // Stop timer
  }

  // Poll controller state and update GUI if changed
  notify();

  return true; // Continue timer
}

} // namespace user_interface

} // namespace Micro_composer
