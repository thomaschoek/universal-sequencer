#include "sequencer/step_sequencer.h"
#include <chrono>
#ifndef NDEBUG
#include <iostream>
#endif
#include <optional>
#include <thread>

namespace MicroComposer {

namespace sequencer {

inline bool StepSequencer::is_live() const {
  return live.load(std::memory_order_relaxed);
}

void StepSequencer::run() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] in Sequencer::run at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  std::optional<Step> step;
  if (sequence.empty()) {
    store_live(false);
    return;
  }
  step = sequence.front();

  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::duration<double>>
      trigger_time = std::chrono::steady_clock::now();

  while (is_live()) {
    // Add the current step's offset to trigger time
    trigger_time += step->offset;

    // Sleep until the next trigger time
    std::this_thread::sleep_until(trigger_time);

    output.write(step.value());

    trigger_time += step->length;

    step = sequence.step();

    if (!step.has_value()) {
      store_live(false);
      return;
    }
  }
}

inline void StepSequencer::store_live(const bool &val) {
  live.store(val, std::memory_order_relaxed);
}

void StepSequencer::start() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::start at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  if (is_live()) {
    return;
  }
  store_live(true);
  thread_ = std::jthread(&StepSequencer::run, this);
}

void StepSequencer::stop() {
#ifndef NDEBUG
  // Print debug message about the exact time the clock started
  auto now = std::chrono::steady_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  std::cout << "[DEBUG] In Sequencer::stop at " << now_ms.count() << " ms..."
            << std::endl;
#endif

  if (is_live()) {
    store_live(false);
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

} // namespace sequencer

} // namespace MicroComposer
