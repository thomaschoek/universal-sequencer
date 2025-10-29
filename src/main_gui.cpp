#include "controller/poly_sequencer_controller.h"
#include "gui/gui.h"
#include "sequencable/premade_samples.h"
#include "synth/synth.h"
#include <atomic>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
  using namespace Micro_composer;
  using namespace Micro_composer::controller;
  using namespace Micro_composer::gui;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  try {
    // Set up audio output - create a pool of synthesizers
    constexpr std::size_t SYNTH_VOICES = 4;
    std::vector<std::unique_ptr<RealTimeAudioOutput>> synth_outputs;
    std::vector<std::unique_ptr<Synthesizer>> synths;

    for (std::size_t i = 0; i < SYNTH_VOICES; ++i) {
      synth_outputs.emplace_back(std::make_unique<RealTimeAudioOutput>());
      synths.emplace_back(std::make_unique<Synthesizer>(*synth_outputs.back()));
    }

    // Create handler factory that uses the synth pool
    // Each sequencer gets a dedicated voice to prevent overlap
    auto handler_factory = [&synths]() {
      static std::atomic<size_t> sequencer_counter{0};
      size_t voice = sequencer_counter.fetch_add(1, std::memory_order_relaxed) % synths.size();
      return [&synths, voice](Premade_samples&& event) {
        synths[voice]->write(event.samples_);
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

    // Enable all events so they can produce sound
    controller.enable();

    // Create and run GUI
    Gui<Premade_samples> gui(controller, 50); // 50 FPS

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
