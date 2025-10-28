#include "controller/poly_sequencer_controller.h"
#include "sequencable/premade_samples.h"
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace Micro_composer;
using namespace Micro_composer::controller;
using namespace Micro_composer::sequencable;

TEST_CASE("Poly_sequencer_controller state initialization", "[controller]") {
  // Create simple handler
  auto handler_factory = []() {
    return [](Premade_samples&& event) {
      // No-op handler for testing
      (void)event;
    };
  };

  // Create sequences
  std::vector<std::vector<Premade_samples>> sequences;
  sequences.push_back({Premade_samples("C4", 0.5), Premade_samples("E4", 0.5)});
  sequences.push_back({Premade_samples("G4", 0.5), Premade_samples("B4", 0.5),
                       Premade_samples("D5", 0.5)});

  Poly_sequencer_controller<Premade_samples> controller(handler_factory,
                                                        sequences);

  SECTION("State vectors are correctly sized") {
    auto state = controller.get_state();
    REQUIRE(state.sizes.size() == 2);
    REQUIRE(state.events.size() == 2);
    REQUIRE(state.positions.size() == 2);
    REQUIRE(state.scheduling.size() == 2);
    REQUIRE(state.t_next.size() == 2);
  }

  SECTION("State contains correct sizes") {
    auto state = controller.get_state();
    REQUIRE(state.sizes[0] == 2);
    REQUIRE(state.sizes[1] == 3);
  }

  SECTION("State contains correct events") {
    auto state = controller.get_state();
    REQUIRE(state.events[0].size() == 2);
    REQUIRE(state.events[1].size() == 3);
    // Check first event frequency
    REQUIRE(state.events[0][0].frequency == Premade_samples::freq_of("C4"));
  }
}

TEST_CASE("Controller selection management", "[controller]") {
  auto handler_factory = []() {
    return [](Premade_samples&& event) { (void)event; };
  };

  std::vector<std::vector<Premade_samples>> sequences;
  sequences.push_back({Premade_samples("C4", 0.5), Premade_samples("E4", 0.5)});
  sequences.push_back({Premade_samples("G4", 0.5), Premade_samples("B4", 0.5)});

  Poly_sequencer_controller<Premade_samples> controller(handler_factory,
                                                        sequences);

  SECTION("Initial selection is empty") {
    REQUIRE_FALSE(controller.selected_seq().has_value());
    REQUIRE_FALSE(controller.selected_event().has_value());
  }

  SECTION("Select sequencer and event") {
    controller.select(1, 1);
    REQUIRE(controller.selected_seq().value() == 1);
    REQUIRE(controller.selected_event().value() == 1);
  }

  SECTION("Select next/prev sequencer") {
    controller.select(0, 0);
    controller.select_next_seq();
    REQUIRE(controller.selected_seq().value() == 1);
    REQUIRE(controller.selected_event().value() == 0); // Resets to 0

    controller.select_prev_seq();
    REQUIRE(controller.selected_seq().value() == 0);
  }

  SECTION("Select next/prev position") {
    controller.select(0, 0);
    controller.select_next_pos();
    REQUIRE(controller.selected_event().value() == 1);

    controller.select_prev_pos();
    REQUIRE(controller.selected_event().value() == 0);
  }

  SECTION("Clear selection") {
    controller.select(1, 1);
    controller.clear_selection();
    REQUIRE_FALSE(controller.selected_seq().has_value());
    REQUIRE_FALSE(controller.selected_event().has_value());
  }
}

TEST_CASE("Controller event modification updates state", "[controller]") {
  auto handler_factory = []() {
    return [](Premade_samples&& event) { (void)event; };
  };

  std::vector<std::vector<Premade_samples>> sequences;
  sequences.push_back({Premade_samples("C4", 0.5), Premade_samples("E4", 0.5)});

  Poly_sequencer_controller<Premade_samples> controller(handler_factory,
                                                        sequences);

  SECTION("Toggle updates state") {
    auto initial_state = controller.get_state();
    bool initial_enabled = initial_state.events[0][0].enabled;

    controller.toggle(0, 0);

    auto updated_state = controller.get_state();
    REQUIRE(updated_state.events[0][0].enabled == !initial_enabled);
  }

  SECTION("Enable updates state") {
    controller.disable(0, 0);
    controller.enable(0, 0);

    auto state = controller.get_state();
    REQUIRE(state.events[0][0].enabled == true);
  }

  SECTION("Disable updates state") {
    controller.enable(0, 0);
    controller.disable(0, 0);

    auto state = controller.get_state();
    REQUIRE(state.events[0][0].enabled == false);
  }
}

TEST_CASE("Controller transport control", "[controller]") {
  auto handler_factory = []() {
    return [](Premade_samples&& event) { (void)event; };
  };

  std::vector<std::vector<Premade_samples>> sequences;
  sequences.push_back({Premade_samples("C4", 0.5), Premade_samples("E4", 0.5)});

  using Clock = Poly_sequencer_controller<Premade_samples>::Clock;

  Poly_sequencer_controller<Premade_samples> controller(handler_factory,
                                                        sequences);

  SECTION("Start sequencer") {
    REQUIRE_FALSE(controller.is_scheduling(0));
    auto start_time = Clock::now() + std::chrono::milliseconds(50);
    controller.start(0, start_time);
    // Wait a bit for scheduler thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(controller.is_scheduling(0));
    controller.stop(0);
  }

  SECTION("Pause sequencer") {
    auto start_time = Clock::now() + std::chrono::milliseconds(50);
    controller.start(0, start_time);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(controller.is_scheduling(0));

    controller.pause(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(controller.is_scheduling(0));
  }

  SECTION("Stop sequencer") {
    auto start_time = Clock::now() + std::chrono::milliseconds(50);
    controller.start(0, start_time);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(controller.is_scheduling(0));

    controller.stop(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(controller.is_scheduling(0));

    // Position should be reset to 0
    auto state = controller.get_state();
    REQUIRE(state.positions[0] == 0);
  }
}
