#include "gui/gui.h"

namespace Micro_composer {

namespace gui {

// Define the event loop as a static method that receives a reference to a Gui
// instance or whatever instances it needs to operate on
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::event_loop(Gui& gui, Controller& controller,
                              unsigned int fps) {
  // 1. Check for input input events (keyboard, elements clicked on screen, etc)
  // 2. If there were any input events, send them to the controller to be
  // handled by controller functions mapped to those events
  // 3. If during the handling of input events the GUI state was changed,
  // re-render the GUI using a reference to the new GUI state that has already
  // been updated during input handling sleep for 1/fps
  // std::this_thread::sleep_for(1 / fps); // cast `1 / fps` to the appropriate
  // duration type
}

} // namespace gui

} // namespace Micro_composer
