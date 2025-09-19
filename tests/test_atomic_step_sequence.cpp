#include "sequence/atomic_step_sequence.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace MicroComposer::sequencer;

// Helper function to create a Step with specific parameters
Step createStep(double offset_seconds, double length_seconds,
                std::vector<double> params = {}) {
  Step step;
  step.offset = std::chrono::duration<double>(offset_seconds);
  step.length = std::chrono::duration<double>(length_seconds);
  step.parameters = std::move(params);
  return step;
}

TEST_CASE("AtomicStepSequence basic operations", "[atomic_step_sequence]") {
  SECTION("Default construction and empty sequence") {
    AtomicStepSequence sequence(0);
    REQUIRE(sequence.empty());
    REQUIRE(sequence.size() == 0);

    auto step = sequence.step();
    REQUIRE_FALSE(step.has_value());
  }

  SECTION("Construction with size") {
    AtomicStepSequence sequence(3);
    REQUIRE(sequence.size() == 3);
    REQUIRE_FALSE(sequence.empty());

    // Default-constructed Steps should have zero offset and 1 second length
    for (size_t i = 0; i < 3; ++i) {
      REQUIRE(sequence[i].offset.count() == 0.0);
      REQUIRE(sequence[i].length.count() == 1.0);
      REQUIRE(sequence[i].parameters.empty());
    }
  }
}

TEST_CASE("AtomicStepSequence step iteration", "[atomic_step_sequence]") {
  AtomicStepSequence sequence(0);

  // Add some test steps
  sequence.push_back(createStep(0.0, 0.5, {1.0, 2.0}));
  sequence.push_back(createStep(0.1, 0.3, {3.0, 4.0}));
  sequence.push_back(createStep(0.2, 0.7, {5.0, 6.0}));

  SECTION("Sequential step iteration") {
    auto step1 = sequence.step();
    REQUIRE(step1.has_value());
    REQUIRE(step1->offset.count() == 0.0);
    REQUIRE(step1->length.count() == 0.5);
    REQUIRE(step1->parameters.size() == 2);
    REQUIRE(step1->parameters[0] == 1.0);
    REQUIRE(step1->parameters[1] == 2.0);

    auto step2 = sequence.step();
    REQUIRE(step2.has_value());
    REQUIRE(step2->offset.count() == 0.1);
    REQUIRE(step2->length.count() == 0.3);
    REQUIRE(step2->parameters[0] == 3.0);
    REQUIRE(step2->parameters[1] == 4.0);

    auto step3 = sequence.step();
    REQUIRE(step3.has_value());
    REQUIRE(step3->offset.count() == 0.2);
    REQUIRE(step3->length.count() == 0.7);
    REQUIRE(step3->parameters[0] == 5.0);
    REQUIRE(step3->parameters[1] == 6.0);

    // Should wrap around to the beginning
    auto step4 = sequence.step();
    REQUIRE(step4.has_value());
    REQUIRE(step4->offset.count() == 0.0);
    REQUIRE(step4->parameters[0] == 1.0);
    REQUIRE(step4->parameters[1] == 2.0);
  }

  SECTION("Iterator resets when sequence becomes empty") {
    // Remove all elements
    sequence.clear();
    REQUIRE(sequence.empty());

    auto step = sequence.step();
    REQUIRE_FALSE(step.has_value());

    // Add elements back
    sequence.push_back(createStep(1.0, 2.0, {10.0}));

    // Should start from beginning again
    auto newStep = sequence.step();
    REQUIRE(newStep.has_value());
    REQUIRE(newStep->offset.count() == 1.0);
    REQUIRE(newStep->parameters[0] == 10.0);
  }
}

TEST_CASE("AtomicStepSequence inheritance from AtomicDeque",
          "[atomic_step_sequence]") {
  AtomicStepSequence sequence(0);

  SECTION("Can use AtomicDeque operations") {
    auto step1 = createStep(0.1, 0.5, {1.0});
    auto step2 = createStep(0.2, 0.6, {2.0});
    auto step3 = createStep(0.3, 0.7, {3.0});

    // Test push operations
    sequence.push_back(step1);
    sequence.push_front(step2);
    sequence.push_back(step3);

    REQUIRE(sequence.size() == 3);
    REQUIRE(sequence.front().parameters[0] == 2.0); // step2 was pushed to front
    REQUIRE(sequence.back().parameters[0] == 3.0);  // step3 was pushed to back

    // Test access operations
    REQUIRE(sequence[0].parameters[0] == 2.0);
    REQUIRE(sequence[1].parameters[0] == 1.0);
    REQUIRE(sequence[2].parameters[0] == 3.0);

    REQUIRE(sequence.at(0).parameters[0] == 2.0);
    REQUIRE(sequence.at(1).parameters[0] == 1.0);
    REQUIRE(sequence.at(2).parameters[0] == 3.0);

    // Test pop operations
    sequence.pop_front();
    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence.front().parameters[0] == 1.0);

    sequence.pop_back();
    REQUIRE(sequence.size() == 1);
    REQUIRE(sequence.back().parameters[0] == 1.0);
  }
}

TEST_CASE("AtomicStepSequence complex step patterns",
          "[atomic_step_sequence]") {
  AtomicStepSequence sequence(0);

  SECTION("Different step configurations") {
    // Create steps with various timing and parameters
    sequence.push_back(createStep(0.0, 0.25, {60, 127})); // Note, velocity
    sequence.push_back(createStep(0.25, 0.25, {62, 100}));
    sequence.push_back(createStep(0.5, 0.5, {64, 80}));
    sequence.push_back(createStep(1.0, 0.25, {})); // Rest step (no parameters)

    REQUIRE(sequence.size() == 4);

    // Test that all steps can be retrieved correctly
    for (int i = 0; i < 8; ++i) { // Test two full cycles
      auto step = sequence.step();
      REQUIRE(step.has_value());

      switch (i % 4) {
      case 0:
        REQUIRE(step->offset.count() == 0.0);
        REQUIRE(step->length.count() == 0.25);
        REQUIRE(step->parameters.size() == 2);
        REQUIRE(step->parameters[0] == 60);
        REQUIRE(step->parameters[1] == 127);
        break;
      case 1:
        REQUIRE(step->offset.count() == 0.25);
        REQUIRE(step->length.count() == 0.25);
        REQUIRE(step->parameters[0] == 62);
        REQUIRE(step->parameters[1] == 100);
        break;
      case 2:
        REQUIRE(step->offset.count() == 0.5);
        REQUIRE(step->length.count() == 0.5);
        REQUIRE(step->parameters[0] == 64);
        REQUIRE(step->parameters[1] == 80);
        break;
      case 3:
        REQUIRE(step->offset.count() == 1.0);
        REQUIRE(step->length.count() == 0.25);
        REQUIRE(step->parameters.empty());
        break;
      }
    }
  }
}

TEST_CASE("AtomicStepSequence thread safety", "[atomic_step_sequence]") {
  AtomicStepSequence sequence(0);

  // Pre-populate with some steps
  for (int i = 0; i < 10; ++i) {
    sequence.push_back(createStep(i * 0.1, 0.1, {static_cast<double>(i)}));
  }

  SECTION("Concurrent step() calls") {
    const int num_threads = 4;
    const int steps_per_thread = 25;
    std::vector<std::future<std::vector<Step>>> futures;

    // Launch multiple threads that call step()
    for (int t = 0; t < num_threads; ++t) {
      futures.push_back(
          std::async(std::launch::async, [&sequence, steps_per_thread]() {
            std::vector<Step> steps;
            for (int i = 0; i < steps_per_thread; ++i) {
              auto step = sequence.step();
              if (step.has_value()) {
                steps.push_back(step.value());
              }
            }
            return steps;
          }));
    }

    // Collect all results
    std::vector<Step> all_steps;
    for (auto &future : futures) {
      auto steps = future.get();
      all_steps.insert(all_steps.end(), steps.begin(), steps.end());
    }

    // Should have collected steps from all threads
    REQUIRE(all_steps.size() == num_threads * steps_per_thread);

    // Verify that steps are valid (they should cycle through the sequence)
    for (const auto &step : all_steps) {
      bool found = false;
      for (size_t i = 0; i < sequence.size(); ++i) {
        if (step.parameters.size() > 0 &&
            step.parameters[0] == static_cast<double>(i)) {
          found = true;
          break;
        }
      }
      REQUIRE(found);
    }
  }

  SECTION("Concurrent modification and step() calls") {
    std::atomic<bool> stop_flag{false};
    std::vector<std::future<int>> futures;

    // Producer thread - adds new steps
    futures.push_back(std::async(std::launch::async, [&sequence, &stop_flag]() {
      int added = 0;
      while (!stop_flag.load()) {
        sequence.push_back(
            createStep(0.1, 0.1, {static_cast<double>(added + 100)}));
        added++;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
      return added;
    }));

    // Consumer thread - calls step()
    futures.push_back(std::async(std::launch::async, [&sequence, &stop_flag]() {
      int consumed = 0;
      while (!stop_flag.load()) {
        auto step = sequence.step();
        if (step.has_value()) {
          consumed++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(5));
      }
      return consumed;
    }));

    // Let them run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop_flag.store(true);

    // Wait for completion and verify no crashes occurred
    for (auto &future : futures) {
      REQUIRE_NOTHROW(future.get());
    }
  }

  SECTION("Thread-safe size checks during step iteration") {
    std::vector<std::future<bool>> futures;
    const int num_threads = 3;

    for (int t = 0; t < num_threads; ++t) {
      futures.push_back(std::async(std::launch::async, [&sequence]() {
        bool success = true;
        for (int i = 0; i < 20; ++i) {
          size_t size_before = sequence.size();
          auto step = sequence.step();
          size_t size_after = sequence.size();

          // Size should remain consistent
          if (size_before != size_after) {
            success = false;
            break;
          }

          // If sequence is not empty, step should have value
          if (size_before > 0 && !step.has_value()) {
            success = false;
            break;
          }
        }
        return success;
      }));
    }

    for (auto &future : futures) {
      REQUIRE(future.get());
    }
  }
}

TEST_CASE("AtomicStepSequence edge cases", "[atomic_step_sequence]") {
  SECTION("Single step sequence") {
    AtomicStepSequence sequence(0);
    auto single_step = createStep(0.5, 1.0, {42.0});
    sequence.push_back(single_step);

    // Should repeatedly return the same step
    for (int i = 0; i < 5; ++i) {
      auto step = sequence.step();
      REQUIRE(step.has_value());
      REQUIRE(step->offset.count() == 0.5);
      REQUIRE(step->length.count() == 1.0);
      REQUIRE(step->parameters[0] == 42.0);
    }
  }

  SECTION("Steps with extreme timing values") {
    AtomicStepSequence sequence(0);

    // Very short step
    sequence.push_back(createStep(0.001, 0.001, {1.0}));
    // Very long step
    sequence.push_back(createStep(10.0, 30.0, {2.0}));
    // Zero-length step
    sequence.push_back(createStep(0.0, 0.0, {3.0}));

    auto step1 = sequence.step();
    REQUIRE(step1.has_value());
    REQUIRE(step1->offset.count() == 0.001);
    REQUIRE(step1->length.count() == 0.001);

    auto step2 = sequence.step();
    REQUIRE(step2.has_value());
    REQUIRE(step2->offset.count() == 10.0);
    REQUIRE(step2->length.count() == 30.0);

    auto step3 = sequence.step();
    REQUIRE(step3.has_value());
    REQUIRE(step3->offset.count() == 0.0);
    REQUIRE(step3->length.count() == 0.0);
  }

  SECTION("Steps with many parameters") {
    AtomicStepSequence sequence(0);

    std::vector<double> many_params;
    for (int i = 0; i < 100; ++i) {
      many_params.push_back(i * 0.1);
    }

    sequence.push_back(createStep(0.1, 0.2, many_params));

    auto step = sequence.step();
    REQUIRE(step.has_value());
    REQUIRE(step->parameters.size() == 100);
    REQUIRE(step->parameters[0] == 0.0);
    REQUIRE(step->parameters[99] == 9.9);
  }
}