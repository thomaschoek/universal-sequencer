#include "controller/poly_sequencer_controller.h"
#include "gui/event_parameter_traits_midi.h"
#include "midi/midi_output.h"
#include "sequencable/midi_event.h"
// Include MIDI headers before gui.h so gui.tpp can use them
#include "gui/gui.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::controller;
using namespace Micro_composer::midi;
using namespace Micro_composer::sequencable;
using namespace Micro_composer::gui;

// Port selection result for one sequencer
struct SequencerPortSelection {
  int port_index;
  std::string port_name;
};

// Display available MIDI ports and let user choose ports for all sequencers
std::vector<SequencerPortSelection> select_midi_ports_for_sequencers(int num_sequencers) {
  auto ports = Midi_output::list_ports();

  if (ports.empty()) {
    std::cerr << "Error: No MIDI output ports found!" << std::endl;
    std::cerr << "Make sure ALSA is configured and a MIDI synthesizer is "
                 "running."
              << std::endl;
    std::cerr << "Try starting yoshimi: yoshimi -a" << std::endl;
    return {};
  }

  std::cout << "\nAvailable MIDI Output Ports:\n" << std::endl;
  for (size_t i = 0; i < ports.size(); ++i) {
    std::cout << "  [" << i << "] " << ports[i].display_name << std::endl;
  }

  std::vector<SequencerPortSelection> selections;

  std::cout << "\nAssign MIDI ports to sequencers:" << std::endl;
  std::cout << "(You can assign the same port to multiple sequencers)\n" << std::endl;

  for (int seq = 0; seq < num_sequencers; ++seq) {
    std::cout << "Sequencer " << seq << " - Select port number (0-"
              << (ports.size() - 1) << ", or -1 to exit): ";
    int selection;
    std::cin >> selection;

    if (selection < 0 || selection >= static_cast<int>(ports.size())) {
      std::cout << "Invalid selection. Exiting..." << std::endl;
      return {};
    }

    selections.push_back({selection, ports[selection].display_name});
    std::cout << "  → Assigned to: " << ports[selection].display_name << "\n" << std::endl;
  }

  return selections;
}

int main(int argc, char** argv) {
  try {
    std::cout << "MicroComposer MIDI Sequencer" << std::endl;
    std::cout << "============================\n" << std::endl;

    // Create test sequences first (to know how many sequencers we need)
    std::vector<std::vector<Midi_event>> sequences;

    // Create 4 sequences (all start with same scale pattern)
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

    const int num_sequencers = sequences.size();

    // Port selection - one port per sequencer
    auto port_selections = select_midi_ports_for_sequencers(num_sequencers);
    if (port_selections.empty()) {
      std::cout << "Exiting..." << std::endl;
      return 0;
    }

    // Create MIDI output instances (one per sequencer)
    std::vector<std::shared_ptr<Midi_output>> midi_outputs;
    for (const auto& selection : port_selections) {
      auto midi_output = std::make_shared<Midi_output>();
      try {
        midi_output->open_port(selection.port_index);
        midi_outputs.push_back(midi_output);
      } catch (const std::exception& e) {
        std::cerr << "Failed to open MIDI port " << selection.port_name
                  << ": " << e.what() << std::endl;
        return 1;
      }
    }

    // Create handler for each sequencer (each with its own MIDI output)
    using Handler = std::function<void(Midi_event&&)>;
    std::vector<Handler> handlers;

    for (auto& midi_output : midi_outputs) {
      handlers.push_back([midi_output](Midi_event&& event) {
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
      });
    }

    // Create controller with per-sequencer handlers
    Poly_sequencer_controller<Midi_event> controller(handlers, sequences);

    // Enable all events
    controller.enable();

    // Create MIDI configuration for GUI
    std::vector<std::string> port_names;
    for (const auto& selection : port_selections) {
      port_names.push_back(selection.port_name);
    }

    // Get available MIDI ports for GUI dropdowns
    auto ports = Midi_output::list_ports();
    std::vector<Gui<Midi_event>::Midi_port_info> available_ports;
    for (size_t i = 0; i < ports.size(); ++i) {
      available_ports.push_back({ports[i].display_name, static_cast<int>(i)});
    }

    Gui<Midi_event>::Midi_config midi_config{midi_outputs, port_names, available_ports};

    // Create and run GUI with MIDI support
    Gui<Midi_event> gui(controller, midi_config, 50); // 50 FPS

    // Print instructions
    std::cout << "Starting MicroComposer GUI..." << std::endl;
    std::cout << "MIDI outputs initialized" << std::endl;
    for (size_t i = 0; i < port_selections.size(); ++i) {
      std::cout << "  Sequencer " << i << ": " << port_selections[i].port_name << std::endl;
    }
    std::cout << "\nControls:" << std::endl;
    std::cout << "  h/j/k/l - Navigate grid (vim-style)" << std::endl;
    std::cout << "  Space   - Toggle play/pause for selected sequencer"
              << std::endl;
    std::cout << "  i       - Enter edit mode" << std::endl;
    std::cout << "  Esc     - Return to normal mode" << std::endl;
    std::cout << "  Port dropdowns in headers - Change MIDI output per sequencer" << std::endl;
    std::cout << "\nWindow title shows current mode (NORMAL/EDIT)" << std::endl;

    gui.run();

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
