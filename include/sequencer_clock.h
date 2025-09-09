#ifndef SEQUENCER_CLOCK_H
#define SEQUENCER_CLOCK_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

namespace MicroComposer_sequencer {

class Clock {
  // Ensures a sequence's events are scheduled on time

  std::atomic<bool> live; // Control variable for clock's run loop
  std::chrono::duration<float> interval =
      std::chrono::seconds{1}; // Time interval between ticks

  std::thread thread_;        // Thread in which the clock runs
  std::condition_variable cv; // Used to notify clock consumers
  std::mutex cv_mutex;        // Mutex used for the condition variable

  void run(); // Defines what happens while the clock is running

public:
  void start(); // Start the clock in a parallel thread
  void stop();  // Stop the clock
  void await(); // Wait for the clock's next tick
};

} // namespace MicroComposer_sequencer
#endif
