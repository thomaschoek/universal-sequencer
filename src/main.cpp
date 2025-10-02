#include "controller/matrix_sequencer_controller.h"
#include "sequencable/vector_event.h"
#include "ui/ui.h"
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
  using namespace Micro_composer;
  using namespace Micro_composer::controller;
  using namespace Micro_composer::user_interface;
  using namespace Micro_composer::sequencable;

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
