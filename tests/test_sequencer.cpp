#include "sequence/atomic_step_sequence.h"
#include "sequencer/step_sequencer.h"
#include "sequencer/step_sequencer_output.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <random>
#include <set>
#include <thread>
#include <vector>

using namespace MicroComposer::sequencer;

// Thread-safe trigger counter for testing
class TriggerCounter {
private:
  std::atomic<int> count_{0};
  mutable std::mutex steps_mutex_;
  std::vector<Step> triggered_steps_;

public:
  void trigger(const Step &step) {
    count_.fetch_add(1, std::memory_order_relaxed);
    std::scoped_lock lock(steps_mutex_);
    triggered_steps_.push_back(step);
  }

  int getCount() const { return count_.load(std::memory_order_relaxed); }

  std::vector<Step> getTriggeredSteps() const {
    std::scoped_lock lock(steps_mutex_);
    return triggered_steps_;
  }

  void reset() {
    count_.store(0, std::memory_order_relaxed);
    std::scoped_lock lock(steps_mutex_);
    triggered_steps_.clear();
  }
};

// Custom Sequencer class for testing that captures triggered steps
class TestSequencerOutput : public StepSequencerOutput {
private:
  TriggerCounter counter_;

public:
  void write(const Step &step) override {
    counter_.trigger(step);
  }

  TriggerCounter& getCounter() { return counter_; }
  const TriggerCounter& getCounter() const { return counter_; }
};

TEST_CASE("Sequencer basic functionality", "[sequencer]") {
  SECTION("Default construction and initial state") {
    AtomicStepSequence seq(0);
    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    REQUIRE_FALSE(sequencer.is_live());
  }

  SECTION("Start and stop with empty sequence") {
    AtomicStepSequence seq(0);
    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    sequencer.start();
    // Should not become live with empty sequence
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE_FALSE(sequencer.is_live());

    sequencer.stop(); // Should be safe to call even if not running
  }

  SECTION("Start and stop with populated sequence") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0})); // Very short steps for fast test
    seq.push_back(Step(0.001, 0.001, {2.0}));

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    sequencer.start();
    REQUIRE(sequencer.is_live());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(sequencer.is_live());

    sequencer.stop();
    REQUIRE_FALSE(sequencer.is_live());
  }

  SECTION("Multiple start calls should be safe") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    sequencer.start();
    REQUIRE(sequencer.is_live());

    sequencer.start(); // Second start should be ignored
    REQUIRE(sequencer.is_live());

    sequencer.stop();
    REQUIRE_FALSE(sequencer.is_live());
  }

  SECTION("Multiple stop calls should be safe") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    sequencer.start();
    REQUIRE(sequencer.is_live());

    sequencer.stop();
    REQUIRE_FALSE(sequencer.is_live());

    sequencer.stop(); // Second stop should be safe
    REQUIRE_FALSE(sequencer.is_live());
  }
}

TEST_CASE("Sequencer step triggering", "[sequencer]") {
  SECTION("Steps are triggered in sequence") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.01, 0.01,
                       {1.0})); // Longer intervals for more predictable timing
    seq.push_back(Step(0.01, 0.01, {2.0}));
    seq.push_back(Step(0.01, 0.01, {3.0}));

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    sequencer.start();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(200)); // Longer test duration
    sequencer.stop();

    // Should have triggered multiple cycles
    REQUIRE(out.getCounter().getCount() > 3);

    auto triggered = out.getCounter().getTriggeredSteps();
    REQUIRE(triggered.size() > 3);

    // Check that steps cycle correctly - but more flexibly since timing might
    // vary
    std::set<double> triggered_values;
    for (const auto &step : triggered) {
      triggered_values.insert(step.parameters[0]);
    }

    // Should see all three values in the triggered steps
    REQUIRE(triggered_values.count(1.0) > 0);
    REQUIRE(triggered_values.count(2.0) > 0);
    REQUIRE(triggered_values.count(3.0) > 0);
  }

  SECTION("Single step loops correctly") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {42.0}));

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    sequencer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sequencer.stop();

    REQUIRE(out.getCounter().getCount() > 1);

    auto triggered = out.getCounter().getTriggeredSteps();
    for (const auto &step : triggered) {
      REQUIRE(step.parameters[0] == 42.0);
    }
  }
}

TEST_CASE("Sequencer thread safety - concurrent modifications",
          "[sequencer][thread_safety]") {
  SECTION("Concurrent step additions during playback") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));
    seq.push_back(Step(0.001, 0.001, {2.0}));

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_modification{false};
    std::atomic<int> steps_added{0};

    // Start the sequencer
    sequencer.start();

    // Thread that adds steps while sequencer is running
    auto modifier_thread = std::async(std::launch::async, [&]() {
      int step_value = 100;
      while (!stop_modification.load()) {
        seq.push_back(Step(0.001, 0.001, {static_cast<double>(step_value++)}));
        steps_added.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    });

    // Let both threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    stop_modification.store(true);
    modifier_thread.wait();
    sequencer.stop();

    // Verify no crashes occurred and steps were triggered
    REQUIRE(out.getCounter().getCount() > 0);
    REQUIRE(steps_added.load() > 0);
    REQUIRE(seq.size() > 2); // Should have added steps
  }

  SECTION("Concurrent step removals during playback") {
    AtomicStepSequence seq(0);

    // Pre-populate with many steps
    for (int i = 1; i <= 20; ++i) {
      seq.push_back(Step(0.001, 0.001, {static_cast<double>(i)}));
    }

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_modification{false};
    std::atomic<int> steps_removed{0};

    sequencer.start();

    // Thread that removes steps while sequencer is running
    auto modifier_thread = std::async(std::launch::async, [&]() {
      while (!stop_modification.load() && !seq.empty()) {
        if (!seq.empty()) {
          seq.pop_back();
          steps_removed.fetch_add(1);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    stop_modification.store(true);
    modifier_thread.wait();
    sequencer.stop();

    // Should handle step removal gracefully
    REQUIRE(out.getCounter().getCount() > 0);
    REQUIRE(steps_removed.load() > 0);
  }

  SECTION("Concurrent step modifications during playback") {
    AtomicStepSequence seq(0);

    for (int i = 1; i <= 5; ++i) {
      seq.push_back(Step(0.001, 0.001, {static_cast<double>(i)}));
    }

    TestSequencerOutput out;

    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_modification{false};
    std::atomic<int> modifications{0};

    sequencer.start();

    // Thread that modifies existing steps
    auto modifier_thread = std::async(std::launch::async, [&]() {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, 4);

      while (!stop_modification.load()) {
        if (seq.size() > 0) {
          int index = dis(gen) % seq.size();
          if (index < seq.size()) {
            seq[index] = Step(0.001, 0.001, {999.0}); // Modify step
            modifications.fetch_add(1);
          }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(150));
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    stop_modification.store(true);
    modifier_thread.wait();
    sequencer.stop();

    REQUIRE(out.getCounter().getCount() > 0);
    REQUIRE(modifications.load() > 0);
  }

  SECTION("Multiple concurrent modifier threads") {
    AtomicStepSequence seq(0);

    // Start with some initial steps
    for (int i = 1; i <= 10; ++i) {
      seq.push_back(Step(0.001, 0.001, {static_cast<double>(i)}));
    }

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_all{false};
    std::vector<std::future<int>> modifier_futures;

    sequencer.start();

    // Adder thread
    modifier_futures.push_back(std::async(std::launch::async, [&]() {
      int count = 0;
      while (!stop_all.load()) {
        seq.push_back(Step(0.001, 0.001, {100.0 + count}));
        count++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
      return count;
    }));

    // Remover thread
    modifier_futures.push_back(std::async(std::launch::async, [&]() {
      int count = 0;
      while (!stop_all.load()) {
        if (seq.size() > 5) { // Keep minimum number of steps
          seq.pop_front();
          count++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
      }
      return count;
    }));

    // Modifier thread
    modifier_futures.push_back(std::async(std::launch::async, [&]() {
      int count = 0;
      std::random_device rd;
      std::mt19937 gen(rd());

      while (!stop_all.load()) {
        if (seq.size() > 0) {
          std::uniform_int_distribution<> dis(0, seq.size() - 1);
          int index = dis(gen);
          seq[index] = Step(0.001, 0.001, {200.0 + count});
          count++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(150));
      }
      return count;
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    stop_all.store(true);

    // Wait for all modifier threads
    int total_operations = 0;
    for (auto &future : modifier_futures) {
      total_operations += future.get();
    }

    sequencer.stop();

    // Verify system remained stable under concurrent load
    REQUIRE(out.getCounter().getCount() > 0);
    REQUIRE(total_operations > 0);
    REQUIRE(seq.size() > 0); // Should still have some steps
  }
}

TEST_CASE("Sequencer thread safety - step copying behavior",
          "[sequencer][thread_safety]") {

  SECTION("Steps are copied safely during iteration") {
    AtomicStepSequence seq(0);

    // Helper function to create large parameter vectors - returns by value
    // (rvalue)
    auto createLargeParams = [](int size) {
      std::vector<double> params;
      for (int i = 0; i < size; ++i) {
        params.push_back(i * 0.1);
      }
      return params;
    };

    seq.push_back(Step(0.001, 0.001, createLargeParams(100)));

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_modifications{false};

    sequencer.start();

    // Thread that modifies the step's parameters while sequencer is running
    auto modifier = std::async(std::launch::async, [&]() {
      while (!stop_modifications.load()) {
        if (seq.size() > 0) {
          // Modify the parameters of the first step directly
          for (auto &param : seq[0].parameters) {
            param += 1000.0; // Make a significant change
          }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    stop_modifications.store(true);
    modifier.wait();
    sequencer.stop();

    // Verify that steps were triggered and no corruption occurred
    REQUIRE(out.getCounter().getCount() > 0);

    auto triggered = out.getCounter().getTriggeredSteps();
    REQUIRE(triggered.size() > 0);

    // Each triggered step should have valid parameter data
    for (const auto &step : triggered) {
      REQUIRE(step.parameters.size() == 100);
      // Parameters should be either original values or modified values
      // but should be consistent within each step (no partial corruption)
      bool is_original = (step.parameters[0] < 100.0);
      bool is_modified = (step.parameters[0] > 1000.0);
      REQUIRE((is_original || is_modified));

      if (is_original) {
        for (size_t i = 0; i < step.parameters.size(); ++i) {
          REQUIRE(step.parameters[i] == i * 0.1);
        }
      } else {
        for (size_t i = 0; i < step.parameters.size(); ++i) {
          REQUIRE(step.parameters[i] == (i * 0.1 + 1000.0));
        }
      }
    }
  }

  SECTION("Iterator safety during sequence modifications") {
    AtomicStepSequence seq(0);

    // Start with a sequence of steps
    for (int i = 1; i <= 5; ++i) {
      seq.push_back(Step(0.001, 0.001, {static_cast<double>(i)}));
    }

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_modifications{false};

    sequencer.start();

    // Thread that continuously clears and repopulates the sequence
    auto modifier = std::async(std::launch::async, [&]() {
      int cycle = 0;
      while (!stop_modifications.load()) {
        seq.clear();
        std::this_thread::sleep_for(std::chrono::microseconds(50));

        // Repopulate with different values
        for (int i = 1; i <= 3; ++i) {
          seq.push_back(
              Step(0.001, 0.001, {static_cast<double>(cycle * 100 + i)}));
        }
        cycle++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    stop_modifications.store(true);
    modifier.wait();
    sequencer.stop();

    // Should handle sequence clearing gracefully without crashes
    // The sequencer should stop when sequence becomes empty and restart when
    // repopulated
    REQUIRE(out.getCounter().getCount() >= 0); // At least no crashes
  }
}

TEST_CASE("Sequencer thread safety - safe shutdown",
          "[sequencer][thread_safety]") {

  SECTION("Safe shutdown during heavy concurrent modifications") {
    AtomicStepSequence seq(0);

    for (int i = 1; i <= 5; ++i) {
      seq.push_back(Step(0.001, 0.001, {static_cast<double>(i)}));
    }

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    std::atomic<bool> modifier_running{true};
    std::vector<std::future<void>> modifier_futures;

    sequencer.start();

    // Start heavy modification workload
    for (int t = 0; t < 3; ++t) {
      modifier_futures.push_back(std::async(std::launch::async, [&, t]() {
        while (modifier_running.load()) {
          seq.push_back(Step(0.001, 0.001, {static_cast<double>(t * 1000)}));
          if (seq.size() > 20) {
            seq.pop_front();
          }
          std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
      }));
    }

    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Stop sequencer while modifications are happening
    sequencer.stop();

    // Stop modifiers after sequencer is stopped
    modifier_running.store(false);
    for (auto &future : modifier_futures) {
      future.wait();
    }

    // Verify clean shutdown
    REQUIRE_FALSE(sequencer.is_live());
    REQUIRE(out.getCounter().getCount() > 0);
  }

  SECTION("Rapid start/stop cycles with concurrent modifications") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));
    seq.push_back(Step(0.001, 0.001, {2.0}));

    TestSequencerOutput out;
    StepSequencer sequencer(seq, out);

    std::atomic<bool> stop_test{false};

    // Thread that continuously modifies sequence
    auto modifier = std::async(std::launch::async, [&]() {
      int value = 100;
      while (!stop_test.load()) {
        seq.push_back(Step(0.001, 0.001, {static_cast<double>(value++)}));
        if (seq.size() > 10) {
          seq.pop_front();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      }
    });

    // Rapidly start and stop sequencer
    for (int i = 0; i < 5; ++i) {
      sequencer.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      sequencer.stop();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    stop_test.store(true);
    modifier.wait();

    // Should be in stopped state
    REQUIRE_FALSE(sequencer.is_live());
  }
}

TEST_CASE("Sequencer destructor safety", "[sequencer][thread_safety]") {

  SECTION("Destructor safely stops running sequencer") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));

    TestSequencerOutput out;

    {
      StepSequencer sequencer(seq, out);
      sequencer.start();
      REQUIRE(sequencer.is_live());

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // Destructor should safely stop the sequencer
    }

    // After destructor, sequencer should have stopped cleanly
    REQUIRE(out.getCounter().getCount() > 0);
  }

  SECTION("Destructor with concurrent modifications") {
    AtomicStepSequence seq(0);
    seq.push_back(Step(0.001, 0.001, {1.0}));

    std::atomic<bool> stop_modifier{false};
    TestSequencerOutput out;

    auto modifier = std::async(std::launch::async, [&]() {
      int value = 100;
      while (!stop_modifier.load()) {
        seq.push_back(Step(0.001, 0.001, {static_cast<double>(value++)}));
        if (seq.size() > 5) {
          seq.pop_front();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    });

    {
      StepSequencer sequencer(seq, out);
      sequencer.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      // Destructor executes here with modifier still running
    }

    stop_modifier.store(true);
    modifier.wait();

    REQUIRE(out.getCounter().getCount() > 0);
  }
}