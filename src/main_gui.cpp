#include "audio/simple_audio_output.h"
#include "controller/poly_sequencer_controller.h"
#include "gui/gui.h"
#include "sequencable/premade_samples.h"
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
  using namespace Micro_composer;
  using namespace Micro_composer::audio;
  using namespace Micro_composer::controller;
  using namespace Micro_composer::gui;
  using namespace Micro_composer::sequencable;

  try {
    // Create audio output
    Simple_audio_output audio_out;

    // Create event handler that plays audio
    auto handler_factory = [&audio_out]() {
      return [&audio_out](Premade_samples&& event) {
        audio_out.play_samples(event.samples_);
      };
    };

    // Create initial sequences for 4 sequencers
    std::vector<std::vector<Premade_samples>> sequences;

    // Sequencer 1: C major chord progression
    sequences.push_back({
        Premade_samples("C4", 0.5), // C
        Premade_samples("E4", 0.5), // E
        Premade_samples("G4", 0.5), // G
        Premade_samples("C5", 0.5), // C
    });

    // Sequencer 2: Bass line
    sequences.push_back({
        Premade_samples("C2", 0.6),  // C
        Premade_samples("C2", 0.6),  // C
        Premade_samples("G2", 0.6),  // G
        Premade_samples("G2", 0.6),  // G
        Premade_samples("A2", 0.6),  // A
        Premade_samples("A2", 0.6),  // A
        Premade_samples("F2", 0.6),  // F
        Premade_samples("F2", 0.6),  // F
    });

    // Sequencer 3: Melody
    sequences.push_back({
        Premade_samples("E5", 0.4),  // E
        Premade_samples("D5", 0.4),  // D
        Premade_samples("C5", 0.4),  // C
        Premade_samples("D5", 0.4),  // D
        Premade_samples("E5", 0.4),  // E
        Premade_samples("E5", 0.4),  // E
        Premade_samples("E5", 0.4),  // E
        Premade_samples("D5", 0.4),  // D
        Premade_samples("D5", 0.4),  // D
        Premade_samples("D5", 0.4),  // D
        Premade_samples("E5", 0.4),  // E
        Premade_samples("G5", 0.4),  // G
    });

    // Sequencer 4: High harmony
    sequences.push_back({
        Premade_samples("G5", 0.3),  // G
        Premade_samples("A5", 0.3),  // A
        Premade_samples("G5", 0.3),  // G
        Premade_samples("F5", 0.3),  // F
        Premade_samples("E5", 0.3),  // E
        Premade_samples("D5", 0.3),  // D
        Premade_samples("C5", 0.3),  // C
        Premade_samples("D5", 0.3),  // D
    });

    // Create controller with sequences (state is initialized automatically)
    Poly_sequencer_controller<Premade_samples> controller(handler_factory,
                                                          sequences);

    // Create and run GUI
    Gui<Premade_samples> gui(controller, 50); // 50 FPS

    // Print instructions to stderr (stdout is used for audio data)
    std::cerr << "Starting MicroComposer GUI..." << std::endl;
    std::cerr << "Audio: Pipe to aplay for sound: ./MicroComposer | aplay -f "
                 "S16_LE -r 44100 -c 1"
              << std::endl;
    std::cerr << "Controls:" << std::endl;
    std::cerr << "  h/j/k/l - Navigate grid (vim-style)" << std::endl;
    std::cerr << "  Space   - Toggle play/pause for selected sequencer"
              << std::endl;
    std::cerr << "  i       - Enter edit mode" << std::endl;
    std::cerr << "  Esc     - Return to normal mode" << std::endl;
    std::cerr << "Window title shows current mode (NORMAL/EDIT)" << std::endl;

    gui.run();

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
