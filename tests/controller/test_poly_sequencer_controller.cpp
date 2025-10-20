#include "controller/poly_sequencer_controller.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <vector>

using namespace Micro_composer::sequencer;
using namespace Micro_composer::controller;

// Test event type that satisfies Has_duration concept
struct Controller_test_event {
  using Duration = std::chrono::steady_clock::duration;
  Duration dur;

  Controller_test_event() : dur(std::chrono::milliseconds(100)) {}
  explicit Controller_test_event(Duration d) : dur(d) {}

  // Conversion operator to Duration for use in Sequencer
  operator Duration() const { return dur; }
};

// Verify Controller_test_event satisfies Has_duration concept
static_assert(Sequencable<Controller_test_event>,
              "Controller_test_event does not satisfy Has_duration concept");

TEST_CASE("Poly_sequencer_controller selection management",
          "[poly_sequencer_controller]") {
  using Controller = Poly_sequencer_controller<Controller_test_event>;

  // Create test sequences
  std::vector<Controller_test_event> seq1 = {
      Controller_test_event{std::chrono::milliseconds(100)},
      Controller_test_event{std::chrono::milliseconds(200)},
      Controller_test_event{std::chrono::milliseconds(150)}};
  std::vector<Controller_test_event> seq2 = {
      Controller_test_event{std::chrono::milliseconds(100)},
      Controller_test_event{std::chrono::milliseconds(100)}};

  std::vector<std::vector<Controller_test_event>> sequences = {seq1, seq2};

  SECTION("Initial state - no selection") {
    Controller ctrl{sequences};
    REQUIRE(!ctrl.selected_seq().has_value());
    REQUIRE(!ctrl.selected_pos().has_value());
  }

  SECTION("Direct selection") {
    Controller ctrl{sequences};

    ctrl.select(0, 1);
    REQUIRE(ctrl.selected_seq().has_value());
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_pos().has_value());
    REQUIRE(ctrl.selected_pos().value() == 1);

    ctrl.select(1, 0);
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_pos().value() == 0);
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
    REQUIRE(!ctrl.selected_pos().has_value());
  }

  SECTION("Navigate next/prev sequencer") {
    Controller ctrl{sequences};

    // Initially no selection, select_next_seq should select first
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_pos().value() == 0);

    // Move to next sequencer
    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_pos().value() == 0);

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

  SECTION("Navigate next/prev position") {
    Controller ctrl{sequences};
    ctrl.select(0, 0);

    // Move to next position
    ctrl.select_next_pos();
    REQUIRE(ctrl.selected_pos().value() == 1);

    ctrl.select_next_pos();
    REQUIRE(ctrl.selected_pos().value() == 2);

    // Wrap around
    ctrl.select_next_pos();
    REQUIRE(ctrl.selected_pos().value() == 0);

    // Go back
    ctrl.select_prev_pos();
    REQUIRE(ctrl.selected_pos().value() == 2);

    ctrl.select_prev_pos();
    REQUIRE(ctrl.selected_pos().value() == 1);

    ctrl.select_prev_pos();
    REQUIRE(ctrl.selected_pos().value() == 0);

    // Wrap to last
    ctrl.select_prev_pos();
    REQUIRE(ctrl.selected_pos().value() == 2);
  }

  SECTION("Position navigation without seq selection does nothing") {
    Controller ctrl{sequences};
    // No selection yet
    ctrl.select_next_pos();
    REQUIRE(!ctrl.selected_pos().has_value());

    ctrl.select_prev_pos();
    REQUIRE(!ctrl.selected_pos().has_value());
  }

  SECTION("Empty controller") {
    std::vector<std::vector<Controller_test_event>> empty;
    Controller ctrl{empty};

    ctrl.select_next_seq();
    REQUIRE(!ctrl.selected_seq().has_value());

    ctrl.select_prev_seq();
    REQUIRE(!ctrl.selected_seq().has_value());
  }

  SECTION("Changing sequencers resets position to 0") {
    Controller ctrl{sequences};
    ctrl.select(0, 2);
    REQUIRE(ctrl.selected_pos().value() == 2);

    ctrl.select_next_seq();
    REQUIRE(ctrl.selected_seq().value() == 1);
    REQUIRE(ctrl.selected_pos().value() == 0);

    ctrl.select(1, 1);
    ctrl.select_prev_seq();
    REQUIRE(ctrl.selected_seq().value() == 0);
    REQUIRE(ctrl.selected_pos().value() == 0);
  }
}

TEST_CASE("Poly_sequencer_controller inherits base functionality",
          "[poly_sequencer_controller]") {
  using Controller = Poly_sequencer_controller<Controller_test_event>;

  std::vector<Controller_test_event> seq1 = {
      Controller_test_event{std::chrono::milliseconds(100)},
      Controller_test_event{std::chrono::milliseconds(200)}};
  std::vector<Controller_test_event> seq2 = {
      Controller_test_event{std::chrono::milliseconds(150)}};

  std::vector<std::vector<Controller_test_event>> sequences = {seq1, seq2};

  SECTION("Can use base Poly_sequencer methods") {
    Controller ctrl{sequences};

    // Size check
    REQUIRE(ctrl.size() == 2);

    // Access individual sequencers
    REQUIRE(ctrl[0].size() == 2);
    REQUIRE(ctrl[1].size() == 1);

    // Test empty check
    REQUIRE(!ctrl.empty());

    // State queries
    REQUIRE_FALSE(ctrl.any_scheduling());
    REQUIRE_FALSE(ctrl.all_scheduling());
    REQUIRE_FALSE(ctrl.is_scheduling(0));
    REQUIRE_FALSE(ctrl.is_scheduling(1));

    // Set next position
    ctrl.set_next(0, 1);
    ctrl.set_next(1, 0);
  }
}
