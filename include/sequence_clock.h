#ifndef SEQUENCE_CLOCK_H
#define SEQUENCE_CLOCK_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

class SequenceClock {
  // This class will serve as the clock for a sequence so that its events will
  // be sequenced on time

  std::atomic<bool> live; // Used to start or terminate the clock's run loop
  std::chrono::duration<float> interval =
      std::chrono::seconds{1}; // Time interval between ticks

  std::condition_variable cv;

  std::thread thread_;

public:
  void start();
  void run();
  void stop();
};

#endif
