#include "sequencer/matrix_sequencer.tpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;

TEST_CASE("MatrixSequencer basic operations", "[matrix_sequencer]") {
  std::vector<Matrix_sequencer::Sequencer> seqrs;

  seqrs.emplace_back({Vector_event{}, Vector_event{}}, [](Vector_event&& e) {
    std::cout << "Seqr 1 handling event ID: " << e.id << std::endl;
  });

  Matrix_sequencer seqr;

  // Handlers that just print event IDs
  auto handler_1 = [](Vector_event&& event) {
    std::cout << "[Handler 1] Event ID: " << event.id << std::endl;
  };
  auto handler_2 = [](Vector_event&& event) {
    std::cout << "[Handler 2] Event ID: " << event.id << std::endl;
  };

  // Sequences with simple events
  Seq_initializer_list seq_1 = {{1}, {2}, {3}};
  Seq_initializer_list seq_2 = {{4}, {5}, {6}};

  // Add sequences
  seqr.add_sequence(seq_1, handler_1);
  seqr.add_sequence(seq_2, handler_2);

  REQUIRE(seqr.size() == 2);

  // Start all sequencers
  auto start_time =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  seqr.start_all(start_time);

  // Let them run for a short while
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Stop all sequencers
  seqr.stop_all();

  // Clear all sequences
  seqr.clear_all();
  REQUIRE(seqr.size() == 0);
}
