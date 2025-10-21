// Demo: Poly_sequencer_controller with Vector_event
//
// This demonstrates that the new Sequencer/Poly_sequencer_controller
// architecture is compatible with the existing Vector_event type used
// in the GUI.

#include "controller/poly_sequencer_controller.h"
#include "sequencable/vector_event.h"
#include <chrono>
#include <iostream>
#include <thread>

using namespace Micro_composer;
using namespace Micro_composer::controller;
using namespace Micro_composer::sequencable;

int main() {
  using VectorEvent = Vector_event<double>;

  std::cout << "[DEMO] Poly_sequencer_controller with Vector_event\n\n";

  // Verify Vector_event satisfies Has_duration
  static_assert(sequencer::Has_duration<VectorEvent>,
                "Vector_event must satisfy Has_duration concept");
  std::cout << " Vector_event satisfies Has_duration concept\n";

  // Create test sequences
  std::vector<VectorEvent> seq1;
  for (double freq : {261.63, 293.66, 329.63, 349.23}) {
    VectorEvent evt;
    evt.duration = std::chrono::duration<double>(0.25);
    evt.params = {freq, 1.0, 0.0}; // frequency, amplitude, phase
    seq1.push_back(evt);
  }

  std::vector<VectorEvent> seq2;
  for (double freq : {523.25, 493.88, 440.00, 392.00}) {
    VectorEvent evt;
    evt.duration = std::chrono::duration<double>(0.25);
    evt.params = {freq, 0.8, 0.1};
    seq2.push_back(evt);
  }

  std::vector<std::vector<VectorEvent>> sequences = {seq1, seq2};

  // Create controller with sequences
  Poly_sequencer_controller<VectorEvent> controller(sequences);

  std::cout << " Created controller with " << controller.size()
            << " sequences\n";
  std::cout << "  - Sequence 0: " << controller[0].size() << " events\n";
  std::cout << "  - Sequence 1: " << controller[1].size() << " events\n\n";

  // Test selection management
  controller.select(0, 0);
  std::cout << " Selected sequencer " << controller.selected_seq().value()
            << ", position " << controller.selected_pos().value() << "\n";

  controller.select_next_pos();
  std::cout << " Next position: " << controller.selected_pos().value() << "\n";

  controller.select_next_seq();
  std::cout << " Next sequencer: " << controller.selected_seq().value()
            << ", position reset to " << controller.selected_pos().value()
            << "\n\n";

  // Test transport control
  std::cout << "Testing transport control:\n";
  std::cout << "  - Initial state: "
            << (controller.any_scheduling() ? "scheduling" : "stopped") << "\n";

  // Start both sequencers with event handlers
  std::atomic<int> event_count{0};
  auto handler = [&event_count](VectorEvent&& evt) {
    event_count++;
    if (!evt.params.empty()) {
      std::cout << "  j Event " << event_count << ": freq=" << evt.params[0]
                << "Hz, dur=" << evt.duration.count() << "s\n";
    }
  };

  controller.listen(0, handler);
  controller.listen(1, handler);

  auto start_time =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  controller.start_all(start_time, false); // Start all, no repeat

  std::cout << "  - Started all sequencers at t+" << 100 << "ms\n";
  std::cout << "  - Scheduling: "
            << (controller.any_scheduling() ? "yes" : "no") << "\n";

  // Let it run for a bit
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "\n Demo completed successfully!\n";
  std::cout << "  Total events processed: " << event_count << "\n";

  return 0;
}
