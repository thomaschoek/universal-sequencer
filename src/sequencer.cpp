#include "sequencer.h"
#include <chrono>
#include <thread>

bool Sequencer::is_live() const { return live.load(std::memory_order_relaxed); }

void Sequencer::trigger(const Step &step) const {}

void Sequencer::run() const {
  auto step_i = std::begin(steps);
  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::duration<double>>
      trigger_time = std::chrono::steady_clock::now();

  while (is_live() && step_i != std::end(steps)) {
    trigger_time += step_i->offset;

    std::this_thread::sleep_until(trigger_time);

    trigger(*step_i);

    trigger_time += step_i->length;

    if (step_i == std::end(steps) - 1) {
      step_i = std::begin(steps);
    } else {
      ++step_i;
    }
  }
}
