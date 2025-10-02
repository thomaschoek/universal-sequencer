#ifndef MICRO_COMPOSER_USER_INTERFACE_H
#define MICRO_COMPOSER_USER_INTERFACE_H

#include "controller/matrix_sequencer_controller.h"

namespace Micro_composer {

namespace user_interface {

class User_interface {
public:
  // Notify the user about any state change
  void notify();
};

} // namespace user_interface

} // namespace Micro_composer

#endif // MICRO_COMPOSER_USER_INTERFACE_H
