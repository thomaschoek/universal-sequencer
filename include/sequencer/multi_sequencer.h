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
  using Seq_ptr_t = std::unique_ptr<Sequencer_base>;
  using Sequencer_vec = std::vector<Seq_ptr_t>;
  using Seq_idx = Sequencer_vec::size_type;
  using Step_idx = std::size_t;

  template <Sequencable Event_t>
  void add_seq(std::function<void(Event_t&&)>,
               sequence::Atomic_step_sequence<Event_t>&& = {});
  void add_seq(std::unique_ptr<Sequencer_base>&&);
  void drop_seq(const Seq_idx);

  void push_step_back(const Seq_idx, Sequencable auto&&);
  void push_step_front(const Seq_idx, Sequencable auto&&);
  template <Sequencable Event_t> void pop_step_back(const Seq_idx);
  template <Sequencable Event_t> void pop_step_front(const Seq_idx);
  void insert_step(const Seq_idx, const Step_idx, Sequencable auto&&);
  void set_step(const Seq_idx, const Step_idx, Sequencable auto&&);
  template <Sequencable Event_t> void erase_step(const Seq_idx, const Step_idx);
  template <Sequencable Event_t>
  void erase_steps(Seq_idx, const Step_idx first, const Step_idx last);

  void start_all(const Sequencer_time_point = Sequencer_clock::now());
  void stop_all();

  void start(const Seq_idx,
             const Sequencer_time_point = Sequencer_clock::now());
  void stop(const Seq_idx);

private:
  // Sequencers managed by this multi-sequencer that run in parallel
  Sequencer_vec sequencers_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
