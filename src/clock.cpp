#include "../include/sequence_clock.h"
#include <thread>

void SequenceClock::start() {
  if (live.load()) {
    // Already started
    return;
  }
  live.store(true);
  thread_ = std::thread{&SequenceClock::run, this};
}

void SequenceClock::run() {
  while (live.load()) {
    cv.notify_all();
    std::this_thread::sleep_for(interval);
  }
}

void SequenceClock::stop() {
  if (!live.load()) {
    return;
  }
  live.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void SequenceClock::await() {
  std::unique_lock<std::mutex> lock_{cv_mutex};
  return cv.wait(lock_);
}
