#include "controller/poly_sequencer_controller.h"
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
        auto note_off_time = event.scheduled_time + event.duration;

        std::this_thread::sleep_until(note_off_time -
                                      std::chrono::milliseconds(5));
        while (std::chrono::steady_clock::now() <
               note_off_time - std::chrono::milliseconds(2))
          ;
        midi_output->send_message(msgs.note_off);
      };
    };

    // Create test sequences
    std::vector<std::vector<Midi_event>> sequences;

    // Sequence 1: C major scale (C4 to C5)
    std::vector<Midi_event> scale;
    std::vector<std::string> notes = {"C4", "D4", "E4", "F4",
                                      "G4", "A4", "B4", "C5"};
    for (const auto& note : notes) {
      scale.emplace_back(std::chrono::milliseconds(300), note, 80);
    }
    sequences.push_back(std::move(scale));

    // Sequence 2: Bass line (C2, G2)
    sequences.push_back({
        Midi_event(std::chrono::milliseconds(600), "C2", 100),
        Midi_event(std::chrono::milliseconds(600), "G2", 100),
    });

    // Sequence 3: Chord progression (arpeggiated)
    sequences.push_back({
        Midi_event(std::chrono::milliseconds(200), "C4", 70),
        Midi_event(std::chrono::milliseconds(200), "E4", 70),
        Midi_event(std::chrono::milliseconds(200), "G4", 70),
        Midi_event(std::chrono::milliseconds(200), "C5", 70),
    });

    // Create controller
    Poly_sequencer_controller<Midi_event> controller(handler_factory,
                                                     sequences);

    // Enable all events
    controller.enable();

    // Print instructions
    std::cout << "\nSequencer Configuration:" << std::endl;
    std::cout << "  Sequencer 0: C major scale (8 notes)" << std::endl;
    std::cout << "  Sequencer 1: Bass line (2 notes)" << std::endl;
    std::cout << "  Sequencer 2: Arpeggiated chord (4 notes)" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  1-3 : Start sequencer 1-3" << std::endl;
    std::cout << "  p   : Pause all" << std::endl;
    std::cout << "  s   : Stop all" << std::endl;
    std::cout << "  q   : Quit" << std::endl;
    std::cout << "\nPress a key and Enter: ";

    // Simple command loop
    char cmd;
    bool running = true;
    while (running && std::cin >> cmd) {
      switch (cmd) {
      case '1':
        controller.start(0);
        std::cout << "Started sequencer 0" << std::endl;
        break;
      case '2':
        controller.start(1);
        std::cout << "Started sequencer 1" << std::endl;
        break;
      case '3':
        controller.start(2);
        std::cout << "Started sequencer 2" << std::endl;
        break;
      case 'p':
        controller.pause(0);
        controller.pause(1);
        controller.pause(2);
        std::cout << "Paused all sequencers" << std::endl;
        break;
      case 's':
        controller.stop(0);
        controller.stop(1);
        controller.stop(2);
        std::cout << "Stopped all sequencers" << std::endl;
        break;
      case 'q':
        running = false;
        break;
      default:
        std::cout << "Unknown command: " << cmd << std::endl;
      }
      if (running) {
        std::cout << "\nPress a key: ";
      }
    }

    // Stop all sequencers before exiting
    controller.stop(0);
    controller.stop(1);
    controller.stop(2);

    // Give time for note-offs to be sent
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\nShutting down..." << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
