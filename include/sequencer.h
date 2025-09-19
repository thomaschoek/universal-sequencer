#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "atomic_step_sequence.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace sequencer {

class Sequencer {

  std::mutex mutex_;
  std::jthread thread_;
  std::atomic<bool> live{false};
  AtomicStepSequence sequence;
  void run();
  void trigger(const Step &step) const;

public:
  bool is_live() const;
  void store_live(const bool &val);

  void start();
  void stop();

  Sequencer() = default;
  explicit Sequencer(const AtomicStepSequence &seq) : sequence(seq) {}
  ~Sequencer() { stop(); }
};

} // namespace sequencer

#endif
