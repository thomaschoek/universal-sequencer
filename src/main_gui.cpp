#include "controller/poly_sequencer_controller.h"
#include "sequencable/oscillation_event.h"
#include "sequencable/vector_event.h"
#include "synth/synth.h"
#include "ui/ui.h"
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
  using namespace Micro_composer;
  using namespace Micro_composer::controller;
  using namespace Micro_composer::user_interface;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  using Vector_event = Vector_event<double>;
  using Controller = Poly_sequencer_controller<Vector_event>;

  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  std::vector<Vector_event> seq1;
  std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23,
                                      392.00, 440.00, 493.88, 523.25};
  for (auto freq : frequencies1) {
    Vector_event evt;
    evt.data = {freq, 1.0, 0.0}; // frequency, amplitude, phase
    seq1.push_back(evt);
  }

  std::vector<Vector_event> seq2;
  std::vector<double> frequencies2 = {523.25, 493.88, 440.00, 392.00,
                                      349.23, 329.63, 293.66, 261.63};
  for (auto freq : frequencies2) {
    Vector_event evt;
    evt.data = {freq, 0.8, 0.1}; // frequency, amplitude, phase
    seq2.push_back(evt);
  }

  std::vector<std::vector<Vector_event>> sequences = {seq1, seq2};

  // Create controller with sequences
  // Provide a default handler factory that will be replaced later
  auto default_factory = []() -> std::function<void(Vector_event&&)> {
    return [](Vector_event&&) {}; // Empty handler
  };
  auto controller = std::make_shared<Controller>(default_factory, sequences);

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
    return [&synth](const Vector_event&& event) {
      // Convert Vector_event to Oscillation_event for synthesis
      Oscillation_event osc_event;
      if (event.data.size() >= 1)
        osc_event.frequency = event.data[0];
      if (event.data.size() >= 2)
        osc_event.amplitude = event.data[1];
      if (event.data.size() >= 3)
        osc_event.phase = event.data[2];
      osc_event.duration = event.duration;

      synth.play(osc_event);
    };
  };

  // Set handlers for initial sequences
  std::vector<std::function<void(Vector_event&&)>> handlers;
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
    return [synth_ptr = synths[idx].get()](const Vector_event&& event) {
      Oscillation_event osc_event;
      if (event.data.size() >= 1)
        osc_event.frequency = event.data[0];
      if (event.data.size() >= 2)
        osc_event.amplitude = event.data[1];
      if (event.data.size() >= 3)
        osc_event.phase = event.data[2];
      osc_event.duration = event.duration;

      synth_ptr->play(osc_event);
    };
  });

  std::cout << "[INFO] Audio output configured with " << SYNTH_POOL_SIZE
            << " synthesizers" << std::endl;

  // Create user interface with controller
  auto ui = std::make_unique<User_interface<Vector_event>>(controller);

  // Initialize GTK and show window
  ui->init(argc, argv);

  std::cout << "[INFO] Starting GUI..." << std::endl;

  // Run event loop (blocks until window closed)
  ui->run();

  std::cout << "[INFO] GUI closed" << std::endl;

  return 0;
}
