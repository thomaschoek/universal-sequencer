#include "sequencer/step_sequencer.h"
#include <chrono>
#ifndef NDEBUG
#include <iostream>
#endif
#include <optional>
#include <thread>

namespace MicroComposer {

namespace sequencer {

bool Sequencer::is_live() const { return live.load(std::memory_order_relaxed); }

void Sequencer::trigger(const Step &step) const {
#ifndef NDEBUG
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            << ": " << step << std::endl;
#endif
}

void Sequencer::run() {
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

    trigger(step.value());

    trigger_time += step->length;

    step = sequence.step();

    if (!step.has_value()) {
      store_live(false);
      return;
    }
  }
}

void Sequencer::store_live(const bool &val) {
  live.store(val, std::memory_order_relaxed);
}

void Sequencer::start() {
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
  thread_ = std::jthread(&Sequencer::run, this);
}

void Sequencer::stop() {
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
