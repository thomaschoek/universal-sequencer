#include "sequencer/parallel_sequencer_controller.h"
#include "sequencable/oscillation_event.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <vector>

using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;

TEST_CASE("Parallel_sequencer_controller selection management",
          "[parallel_sequencer_controller]") {
  using Controller = Parallel_sequencer_controller<Oscillation_event>;

  // Create test sequences
  std::vector<Oscillation_event> seq1 = {
      Oscillation_event{261.63}, // C4
      Oscillation_event{293.66}, // D4
      Oscillation_event{329.63}  // E4
  };
  std::vector<Oscillation_event> seq2 = {
      Oscillation_event{440.00}, // A4
      Oscillation_event{493.88}  // B4
  };

  std::vector<std::vector<Oscillation_event>> sequences = {seq1, seq2};

  SECTION("Initial state - no selection") {
    Controller ctrl{sequences};
    REQUIRE(!ctrl.selected_seq().has_value());
    REQUIRE(!ctrl.selected_step().has_value());
  }

  SECTION("Direct selection") {
    Controller ctrl{sequences};

    ctrl.select(0, 1);
    REQUIRE(ctrl.selected_seq().has_value());
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_step().has_value());
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

  SECTION("Clear selection") {
    Controller ctrl{sequences};
    ctrl.select(0, 1);
    REQUIRE(ctrl.selected_seq().has_value());

    ctrl.clear_selection();
    REQUIRE(!ctrl.selected_seq().has_value());
    REQUIRE(!ctrl.selected_step().has_value());
  }

  SECTION("Navigate next/prev sequencer") {
    Controller ctrl{sequences};

    // Initially no selection, select_next_seq should select first
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_step().value() == 0);

    // Move to next sequencer
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_step().value() == 0);

    // Wrap around to first
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);

    // Go back
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);

    // Wrap around to last
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
  }

  SECTION("Navigate next/prev step") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);

    // Move to next step
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

  SECTION("Step navigation without seq selection does nothing") {
    Controller ctrl{sequences};
    // No selection yet
    ctrl.select_next_step();
    REQUIRE(!ctrl.selected_step().has_value());

    ctrl.select_prev_step();
    REQUIRE(!ctrl.selected_step().has_value());
  }

  SECTION("Empty controller") {
    std::vector<std::vector<Oscillation_event>> empty;
    Controller ctrl{empty};

    ctrl.select_next_seq();
    REQUIRE(!ctrl.selected_seq().has_value());

    ctrl.select_prev_seq();
    REQUIRE(!ctrl.selected_seq().has_value());
  }

  SECTION("Changing sequencers resets step to 0") {
    Controller ctrl{sequences};
    ctrl.select(0, 2);
    REQUIRE(ctrl.selected_step().value() == 2);

    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_step().value() == 0);

    ctrl.select(1, 1);
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_step().value() == 0);
  }
}

TEST_CASE("Parallel_sequencer_controller inherits base functionality",
          "[parallel_sequencer_controller]") {
  using Controller = Parallel_sequencer_controller<Oscillation_event>;

  std::vector<Oscillation_event> seq1 = {
      Oscillation_event{261.63}, Oscillation_event{293.66}};
  std::vector<Oscillation_event> seq2 = {Oscillation_event{440.00}};

  std::vector<std::vector<Oscillation_event>> sequences = {seq1, seq2};

  SECTION("Can use base Parallel_sequencer methods") {
    Controller ctrl{sequences};

    // Size check
    REQUIRE(ctrl.size() == 2);

    // Access individual sequencers
    REQUIRE(ctrl[0].size() == 2);
    REQUIRE(ctrl[1].size() == 1);

    // Update events
    ctrl.update(0, 0, 440.0, 1.0, 0.0);
    REQUIRE(ctrl[0].at(0).frequency == 440.0);

    // Set handlers (just checking it compiles and doesn't crash)
    std::function<void(Oscillation_event&&)> dummy_handler =
        [](Oscillation_event&&) {};
    ctrl.set_handler(0, dummy_handler);
    ctrl.set_handlers(std::vector{dummy_handler, dummy_handler});
  }
}