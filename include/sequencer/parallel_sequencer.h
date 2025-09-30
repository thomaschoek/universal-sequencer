#ifndef MICRO_COMPOSER_PARALLEL_SEQUENCER_H
#define MICRO_COMPOSER_PARALLEL_SEQUENCER_H

#include "container/atomic_vector.h"
#include "sequencer/atomic_sequencer.h"
#include <chrono>
#include <mutex>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable_updatable Event_t>
class Parallel_sequencer
    : public container::Atomic_vector<Atomic_sequencer<Event_t>> {
public:
  using Sequencer = Atomic_sequencer<Event_t>;
  using Base_vector = container::Atomic_vector<Sequencer>;
  using Seq_idx = typename Base_vector::Index;
  using Clock = typename Sequencer::Clock;
  using Time_point = typename Sequencer::Time_point;

  // Constructors
  Parallel_sequencer() = default;
  Parallel_sequencer(const Parallel_sequencer&) = delete;
  Parallel_sequencer& operator=(const Parallel_sequencer&) = delete;
  Parallel_sequencer(Parallel_sequencer&&) noexcept = default;
  ~Parallel_sequencer() = default;

  Parallel_sequencer(const std::vector<std::vector<Event_t>>&) noexcept;
  Parallel_sequencer(std::vector<std::vector<Event_t>>&&) noexcept;
  Parallel_sequencer(const std::vector<Sequencer>&) noexcept;
  Parallel_sequencer(std::vector<Sequencer>&&) noexcept;

  // Synchronized transport control
  void start(Seq_idx, Time_point start_time = Clock::now());
  void start_all(Time_point start_time = Clock::now());

  void stop(Seq_idx);
  void stop_all();

  bool is_running(Seq_idx) const;
  bool any_running() const;
  bool all_running() const;

private:
  mutable std::mutex transport_mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/parallel_sequencer.tpp"

#endif // MICRO_COMPOSER_PARALLEL_SEQUENCER_H