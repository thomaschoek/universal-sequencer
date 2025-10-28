#include "gui/gui.h"

namespace Micro_composer {

namespace gui {

// Define the event loop for the Graphical User Interface to numerous sequencers
// controlled by a Poly_sequencer_controller
template <sequencable::Mut_seq_event Event_t>
void Gui<Event_t>::event_loop(/*Whatever parameters will be needed*/) {
  // 1. Check for input input events (keyboard, elements clicked on screen, etc)
  // 2. If there were any input events, send them to the controller to be
  // handled by controller functions mapped to those events
  // 3. If during the handling of input events the GUI state was changed,
  // re-render the GUI using a reference to the new GUI state that has already
  // been updated during input handling sleep for 1/fps
  // 4. If no input events were detected, only update the highlighted 'currently
  // playing' cell based on Sequencer::get_pos() for all running sequencers.
  // This should not require a full re-render, but only adding and removign some
  // CSS classes from the relevant cells in the grid. Do anything else you deem
  // appropriate for GUI event loop
}

} // namespace gui

} // namespace Micro_composer
