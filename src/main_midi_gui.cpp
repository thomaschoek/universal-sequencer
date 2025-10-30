#include "controller/poly_sequencer_controller.h"
#include "gui/event_parameter_traits_midi.h"
#include "gui/gui.h"
#include "midi/midi_output.h"
#include "sequencable/midi_event.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::controller;
using namespace Micro_composer::midi;
using namespace Micro_composer::sequencable;
using namespace Micro_composer::gui;

// Display available MIDI ports and let user choose
int select_midi_port() {
  auto ports = Midi_output::list_ports();

  if (ports.empty()) {
    std::cerr << "Error: No MIDI output ports found!" << std::endl;
    std::cerr << "Make sure ALSA is configured and a MIDI synthesizer is "
                 "running."
              << std::endl;
    std::cerr << "Try starting yoshimi: yoshimi -a" << std::endl;
    return -1;
  }

  std::cout << "\nAvailable MIDI Output Ports:\n" << std::endl;
  for (const auto& port : ports) {
    std::cout << "  " << port.display_name << std::endl;
  }

  std::cout << "\nSelect port number (or -1 to exit): ";
  int selection;
  std::cin >> selection;

  if (selection < 0 || selection >= static_cast<int>(ports.size())) {
    return -1;
  }

  return selection;
}

int main(int argc, char** argv) {
  try {
    std::cout << "MicroComposer MIDI Sequencer" << std::endl;
    std::cout << "============================\n" << std::endl;

    // Port selection
    int port_index = select_midi_port();
    if (port_index < 0) {
      std::cout << "Exiting..." << std::endl;
      return 0;
    }

    // Create MIDI output
    auto midi_output = std::make_shared<Midi_output>();
    midi_output->open_port(port_index);

    // Create handler factory that sends MIDI messages
    auto handler_factory = [midi_output]() {
      return [midi_output](Midi_event&& event) {
        // Generate note-on and note-off messages
        auto msgs = event.to_midi_messages();

        // Send note-on immediately
        midi_output->send_message(msgs.note_on);

        // Schedule note-off after duration
        auto duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                event.duration);

        // Spawn a thread to send note-off after delay
        std::thread([midi_output, note_off = msgs.note_off, duration_ms]() {
          std::this_thread::sleep_for(duration_ms);
          midi_output->send_message(note_off);
        });
      };
    };

    // Create test sequences
    std::vector<std::vector<Midi_event>> sequences;

    // Sequence 1: C major scale (C4 to C5)
    std::vector<Midi_event> scale;
    std::vector<std::string> notes = {"C4", "C4", "C4", "C4",
                                      "C4", "C4", "C4", "C4"};
    for (const auto& note : notes) {
      scale.emplace_back(std::chrono::milliseconds(250), note, 64);
    }
    sequences.push_back(scale);
    sequences.push_back(scale);
    sequences.push_back(scale);
    sequences.push_back(std::move(scale));

    // Create controller
    Poly_sequencer_controller<Midi_event> controller(handler_factory,
                                                     sequences);

    // Enable all events
    controller.enable();

    // Create and run GUI
    Gui<Midi_event> gui(controller, 50); // 50 FPS

    // Print instructions
    std::cout << "Starting MicroComposer GUI..." << std::endl;
    std::cout << "Audio output initialized" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  h/j/k/l - Navigate grid (vim-style)" << std::endl;
    std::cout << "  Space   - Toggle play/pause for selected sequencer"
              << std::endl;
    std::cout << "  i       - Enter edit mode" << std::endl;
    std::cout << "  Esc     - Return to normal mode" << std::endl;
    std::cout << "Window title shows current mode (NORMAL/EDIT)" << std::endl;

    gui.run();

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
