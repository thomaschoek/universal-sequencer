#include "controller/poly_sequencer_controller.h"
#include "sequencable/oscillation_event.h"
#include "sequencable/vector_event.h"
#include "synth/synth.h"
#include "ui/ui.h"
#include <future>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
  using namespace Micro_composer;
  using namespace Micro_composer::controller;
  using namespace Micro_composer::user_interface;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  using VectorEvent = Vector_event<double>;

  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  std::vector<VectorEvent> seq1;
  std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23,
                                      392.00, 440.00, 493.88, 523.25};
  for (auto freq : frequencies1) {
    VectorEvent evt;
    evt.params = {freq, 1.0, 0.0}; // frequency, amplitude, phase
    seq1.push_back(evt);
  }

  std::vector<VectorEvent> seq2;
  std::vector<double> frequencies2 = {523.25, 493.88, 440.00, 392.00,
                                      349.23, 329.63, 293.66, 261.63};
  for (auto freq : frequencies2) {
    VectorEvent evt;
    evt.params = {freq, 0.8, 0.1}; // frequency, amplitude, phase
    seq2.push_back(evt);
  }

  std::vector<std::vector<VectorEvent>> sequences = {seq1, seq2};

  // Create controller with sequences
  auto controller =
      std::make_shared<Matrix_sequencer_controller<double>>(sequences);

  std::cout << "[INFO] Created controller with " << controller->size()
            << " sequences" << std::endl;

  // Set up audio output - create a pool of synthesizers
  constexpr std::size_t SYNTH_POOL_SIZE = 10;
  std::vector<std::unique_ptr<RealTimeAudioOutput>> synth_outputs;
  std::vector<std::unique_ptr<Synthesizer>> synths;

  for (std::size_t i = 0; i < SYNTH_POOL_SIZE; ++i) {
    synth_outputs.push_back(std::make_unique<RealTimeAudioOutput>());
    synths.push_back(std::make_unique<Synthesizer>(*synth_outputs.back()));
  }

  // Create handlers that convert Vector_event to Oscillation_event for audio
  // playback
  auto create_handler = [](Synthesizer& synth) {
    return [&synth](const VectorEvent&& event) {
      // Convert Vector_event to Oscillation_event for synthesis
      Oscillation_event osc_event;
      if (event.params.size() >= 1)
        osc_event.frequency = event.params[0];
      if (event.params.size() >= 2)
        osc_event.amplitude = event.params[1];
      if (event.params.size() >= 3)
        osc_event.phase = event.params[2];
      osc_event.duration = event.duration;
      osc_event.offset = event.offset;

      // Play asynchronously to avoid blocking sequencer
      std::ignore = std::async(
          std::launch::async, [&synth, osc_event]() { synth.play(osc_event); });
    };
  };

  // Set handlers for initial sequences
  std::vector<std::function<void(VectorEvent&&)>> handlers;
  for (std::size_t i = 0; i < controller->size() && i < SYNTH_POOL_SIZE; ++i) {
    handlers.push_back(create_handler(*synths[i]));
  }
  controller->set_handlers(handlers);

  // Set up handler factory for dynamically added sequences
  // Use a shared index to cycle through the synth pool
  auto next_synth_idx =
      std::make_shared<std::atomic<std::size_t>>(controller->size());
  controller->set_handler_factory([&synths, next_synth_idx]() {
    std::size_t idx = (*next_synth_idx)++ % synths.size();
    std::cout << "[INFO] Creating handler using synth " << idx << std::endl;

    // Create handler using the lambda from above
    return [synth_ptr = synths[idx].get()](const VectorEvent&& event) {
      Oscillation_event osc_event;
      if (event.params.size() >= 1)
        osc_event.frequency = event.params[0];
      if (event.params.size() >= 2)
        osc_event.amplitude = event.params[1];
      if (event.params.size() >= 3)
        osc_event.phase = event.params[2];
      osc_event.duration = event.duration;
      osc_event.offset = event.offset;

      std::ignore = std::async(std::launch::async, [synth_ptr, osc_event]() {
        synth_ptr->play(osc_event);
      });
    };
  });

  std::cout << "[INFO] Audio output configured with " << SYNTH_POOL_SIZE
            << " synthesizers" << std::endl;

  // Create user interface with controller
  auto ui = std::make_unique<User_interface<double>>(controller);

  // Initialize GTK and show window
  ui->init(argc, argv);

  std::cout << "[INFO] Starting GUI..." << std::endl;

  // Run event loop (blocks until window closed)
  ui->run();

  std::cout << "[INFO] GUI closed" << std::endl;

  return 0;
}
