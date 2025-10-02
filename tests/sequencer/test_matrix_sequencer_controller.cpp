#include "sequencer/matrix_sequencer_controller.h"
#include "sequencable/vector_event.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;

TEST_CASE("Matrix_sequencer_controller parameter selection",
          "[matrix_sequencer_controller]") {
  using Controller = Matrix_sequencer_controller<double>;
  using VectorEvent = Vector_event<double>;

  // Create test sequences with vector events
  // Each event will have 3 parameters: [frequency, amplitude, phase]
  std::vector<VectorEvent> seq1;
  for (int i = 0; i < 3; ++i) {
    VectorEvent evt;
    evt.params = {261.63 + i * 10, 1.0, 0.0}; // 3 parameters each
    seq1.push_back(evt);
  }

  std::vector<VectorEvent> seq2;
  for (int i = 0; i < 2; ++i) {
    VectorEvent evt;
    evt.params = {440.0 + i * 10, 0.5, 0.1}; // 3 parameters each
    seq2.push_back(evt);
  }

  std::vector<std::vector<VectorEvent>> sequences = {seq1, seq2};

  SECTION("Initial state - no selections") {
    Controller ctrl{sequences};
    REQUIRE(!ctrl.selected_seq().has_value());
    REQUIRE(!ctrl.selected_step().has_value());
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Direct parameter selection requires seq/step selection first") {
    Controller ctrl{sequences};
    REQUIRE_THROWS_AS(ctrl.select_param(0), std::runtime_error);

    // Select sequence and step first
    ctrl.select(0, 0);
    REQUIRE_NOTHROW(ctrl.select_param(0));
    REQUIRE(ctrl.selected_param().value() == 0);

    ctrl.select_param(1);
    REQUIRE(ctrl.selected_param().value() == 1);

    ctrl.select_param(2);
    REQUIRE(ctrl.selected_param().value() == 2);
  }

  SECTION("Parameter selection out of range throws") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);

    // Valid range is 0-2 (3 parameters)
    REQUIRE_NOTHROW(ctrl.select_param(2));
    REQUIRE_THROWS_AS(ctrl.select_param(3), std::out_of_range);
  }

  SECTION("Clear parameter selection") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);
    ctrl.select_param(1);
    REQUIRE(ctrl.selected_param().has_value());

    ctrl.clear_param_selection();
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Clear all selections") {
    Controller ctrl{sequences};
    ctrl.select(0, 1);
    ctrl.select_param(2);

    REQUIRE(ctrl.selected_seq().has_value());
    REQUIRE(ctrl.selected_step().has_value());
    REQUIRE(ctrl.selected_param().has_value());

    ctrl.clear_selection();
    REQUIRE(!ctrl.selected_seq().has_value());
    REQUIRE(!ctrl.selected_step().has_value());
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Navigate next/prev parameter") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);

    // Initially no parameter selected, select_next_param should select first
    ctrl.select_next_param();
    REQUIRE(ctrl.selected_param().value() == 0);

    ctrl.select_next_param();
    REQUIRE(ctrl.selected_param().value() == 1);

    ctrl.select_next_param();
    REQUIRE(ctrl.selected_param().value() == 2);

    // Wrap around to first
    ctrl.select_next_param();
    REQUIRE(ctrl.selected_param().value() == 0);

    // Go back
    ctrl.select_prev_param();
    REQUIRE(ctrl.selected_param().value() == 2);

    ctrl.select_prev_param();
    REQUIRE(ctrl.selected_param().value() == 1);

    ctrl.select_prev_param();
    REQUIRE(ctrl.selected_param().value() == 0);

    // Wrap to last
    ctrl.select_prev_param();
    REQUIRE(ctrl.selected_param().value() == 2);
  }

  SECTION("Parameter navigation without seq/step selection does nothing") {
    Controller ctrl{sequences};
    ctrl.select_next_param();
    REQUIRE(!ctrl.selected_param().has_value());

    ctrl.select_prev_param();
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Parameter selection persists across step changes") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);
    ctrl.select_param(1);
    REQUIRE(ctrl.selected_param().value() == 1);

    // Change step
    ctrl.select_next_step();
    REQUIRE(ctrl.selected_step().value() == 1);
    // Parameter selection should persist
    REQUIRE(ctrl.selected_param().value() == 1);
  }

  SECTION("Parameter selection persists across sequence changes") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);
    ctrl.select_param(2);
    REQUIRE(ctrl.selected_param().value() == 2);

    // Change sequence
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    // Parameter selection should persist
    REQUIRE(ctrl.selected_param().value() == 2);
  }
}

TEST_CASE("Matrix_sequencer_controller sequence/step navigation",
          "[matrix_sequencer_controller]") {
  using Controller = Matrix_sequencer_controller<double>;
  using VectorEvent = Vector_event<double>;

  std::vector<VectorEvent> seq1;
  for (int i = 0; i < 3; ++i) {
    VectorEvent evt;
    evt.params = {261.63 + i * 10, 1.0, 0.0};
    seq1.push_back(evt);
  }

  std::vector<VectorEvent> seq2;
  for (int i = 0; i < 2; ++i) {
    VectorEvent evt;
    evt.params = {440.0 + i * 10, 0.5, 0.1};
    seq2.push_back(evt);
  }

  std::vector<std::vector<VectorEvent>> sequences = {seq1, seq2};

  SECTION("Direct selection") {
    Controller ctrl{sequences};

    ctrl.select(0, 1);
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_step().value() == 1);

    ctrl.select(1, 0);
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_step().value() == 0);
  }

  SECTION("Selection out of range throws") {
    Controller ctrl{sequences};
    REQUIRE_THROWS_AS(ctrl.select(2, 0), std::out_of_range);
    REQUIRE_THROWS_AS(ctrl.select(0, 5), std::out_of_range);
  }

  SECTION("Navigate next/prev sequencer") {
    Controller ctrl{sequences};

    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_step().value() == 0);

    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_step().value() == 0);

    // Wrap around
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);

    // Go back
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);

    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);

    // Wrap around
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
  }

  SECTION("Navigate next/prev step") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);

    ctrl.select_next_step();
    REQUIRE(ctrl.selected_step().value() == 1);

    ctrl.select_next_step();
    REQUIRE(ctrl.selected_step().value() == 2);

    // Wrap around
    ctrl.select_next_step();
    REQUIRE(ctrl.selected_step().value() == 0);

    // Go back
    ctrl.select_prev_step();
    REQUIRE(ctrl.selected_step().value() == 2);

    ctrl.select_prev_step();
    REQUIRE(ctrl.selected_step().value() == 1);

    ctrl.select_prev_step();
    REQUIRE(ctrl.selected_step().value() == 0);

    // Wrap to last
    ctrl.select_prev_step();
    REQUIRE(ctrl.selected_step().value() == 2);
  }
}

TEST_CASE("Matrix_sequencer_controller update method",
          "[matrix_sequencer_controller]") {
  using Controller = Matrix_sequencer_controller<double>;
  using VectorEvent = Vector_event<double>;

  std::vector<VectorEvent> seq1;
  for (int i = 0; i < 3; ++i) {
    VectorEvent evt;
    evt.params = {261.63, 1.0, 0.0}; // All same initially
    seq1.push_back(evt);
  }

  std::vector<std::vector<VectorEvent>> sequences = {seq1};

  SECTION("Update specific parameter in specific step") {
    Controller ctrl{sequences};

    // Update parameter 0 (frequency) of step 1 in sequence 0
    ctrl.update(0, 1, 0, 440.0);
    REQUIRE(ctrl[0].at(1).params[0] == 440.0);

    // Update parameter 1 (amplitude) of step 2 in sequence 0
    ctrl.update(0, 2, 1, 0.5);
    REQUIRE(ctrl[0].at(2).params[1] == 0.5);

    // Verify other parameters unchanged
    REQUIRE(ctrl[0].at(0).params[0] == 261.63);
    REQUIRE(ctrl[0].at(1).params[1] == 1.0);
  }

  SECTION("Update out of range throws") {
    Controller ctrl{sequences};

    REQUIRE_THROWS_AS(ctrl.update(1, 0, 0, 100.0), std::out_of_range);
    REQUIRE_THROWS_AS(ctrl.update(0, 5, 0, 100.0), std::out_of_range);
    REQUIRE_THROWS_AS(ctrl.update(0, 0, 5, 100.0), std::out_of_range);
  }
}

TEST_CASE("Matrix_sequencer_controller empty sequences",
          "[matrix_sequencer_controller]") {
  using Controller = Matrix_sequencer_controller<double>;
  using VectorEvent = Vector_event<double>;

  SECTION("Empty controller") {
    std::vector<std::vector<VectorEvent>> empty;
    Controller ctrl{empty};

    ctrl.select_next_seq();
    REQUIRE(!ctrl.selected_seq().has_value());

    ctrl.select_next_param();
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Sequence with empty steps") {
    std::vector<VectorEvent> empty_seq;
    std::vector<std::vector<VectorEvent>> sequences = {empty_seq};
    Controller ctrl{sequences};

    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().has_value());

    ctrl.select_next_step();
    REQUIRE(!ctrl.selected_step().has_value());

    ctrl.select_next_param();
    REQUIRE(!ctrl.selected_param().has_value());
  }

  SECTION("Step with empty params") {
    VectorEvent evt;
    evt.params = {}; // Empty params
    std::vector<VectorEvent> seq = {evt};
    std::vector<std::vector<VectorEvent>> sequences = {seq};
    Controller ctrl{sequences};

    ctrl.select(0, 0);
    ctrl.select_next_param();
    REQUIRE(!ctrl.selected_param().has_value());
  }
}
