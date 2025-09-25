#ifndef MICRO_COMPOSER_MULTI_SEQUENCER_H
#define MICRO_COMPOSER_MULTI_SEQUENCER_H

#include "sequencable/concept.h"
#include "sequence/atomic_step_sequence.h"
#include "sequencer/sequencer_base.h"
#include <functional>
#include <memory>
#include <vector>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

class Multi_sequencer {
public:
  using Sequencer_clock = Sequencer_base::clock;
  using Sequencer_time_point = Sequencer_base::time_point;
  using Sequencer_vec = std::vector<std::unique_ptr<Sequencer_base>>;
  using Seqr_idx = Sequencer_vec::size_type;

  template <Sequencable Event_t>
  void add_seq(std::function<void(Event_t&&)>,
               sequence::Atomic_step_sequence<Event_t>&& = {});
  void add_seq(std::unique_ptr<Sequencer_base>&&);
  void remove_seq(Seqr_idx);

  void push_back(Seqr_idx, Sequencable auto&&);
  void push_front(Seqr_idx, Sequencable auto&&);
  template <Sequencable Event_t> void pop_back(Seqr_idx);
  template <Sequencable Event_t> void pop_front(Seqr_idx);
  void insert(Seqr_idx, std::size_t step_index, Sequencable auto&&);
  void update(Seqr_idx, std::size_t step_index, Sequencable auto&&);
  template <Sequencable Event_t> void erase(Seqr_idx, std::size_t step_index);

  void start_all(Sequencer_time_point = Sequencer_clock::now());
  void stop_all();

  void start(Seqr_idx sequencer_index,
             Sequencer_time_point = Sequencer_clock::now());
  void stop(Seqr_idx sequencer_index);

private:
  // Sequencers managed by this multi-sequencer that run in parallel
  Sequencer_vec sequencers_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
