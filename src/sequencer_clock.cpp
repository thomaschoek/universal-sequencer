#include "../include/sequencer_clock.h"
#include <thread>

namespace MicroComposer_sequencer {

void Clock::start() {
  if (live.load()) {
    // Already started
    return;
  }
  live.store(true);
  thread_ = std::thread{&Clock::run, this};
}

void Clock::run() {
  while (live.load()) {
    cv.notify_all();
    std::this_thread::sleep_for(interval);
  }
}

void Clock::stop() {
  if (!live.load()) {
    return;
  }
  live.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Clock::await() {
  std::unique_lock<std::mutex> lock_{cv_mutex};
  return cv.wait(lock_);
}

} // namespace MicroComposer_sequencer
