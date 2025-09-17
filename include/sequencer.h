#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <atomic>
#include <chrono>
#include <vector>

struct Step {
  std::chrono::duration<double> offset;
  std::chrono::duration<double> length;
  std::vector<double> parameters;
};

class Sequencer {

private:
  std::atomic<bool> live{false};
  void run() const;
  void trigger(const Step &step) const;

public:
  bool is_live() const;

  std::vector<Step> steps;

  void start() const;
  void stop() const;
};

#endif
