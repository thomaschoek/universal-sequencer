#ifndef MICRO_COMPOSER_PARALLEL_SEQUENCER_H
#define MICRO_COMPOSER_PARALLEL_SEQUENCER_H

#include "sequencer/atomic_sequencer.h"
#include <initializer_list>
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable_updatable T_event> class Parallel_sequencer {
public:
  using Sequencer = Atomic_sequencer<T_event>;
  using Sequencer_vector = std::vector<Sequencer>;
  using Steps = Sequencer::Base_deque;
  using Seq_idx = Sequencer_vector::size_type;
  using Step_idx = Sequencer::Step_idx;
  using Handler = Sequencer::Handler;
  using Sequence_initializer_list = Sequencer::Initializer_list;
  using Clock = Sequencer::Clock;
  using Time_point = Sequencer::Time_point;
  using Step_iterator = Sequencer::Step_iterator;

  void start(Seq_idx, Time_point = Clock::now());
  void start_all(Time_point = Clock::now());
  void stop(Seq_idx);
  void stop_all();

  // Sequence-level CRUD
  void add_sequence(Sequence_initializer_list);
  void add_sequence(Sequence_initializer_list, Handler);

  const Steps& get(Seq_idx) const;

  void assign(Seq_idx, std::initializer_list<T_event>);

  void drop_sequence(Seq_idx);
  void set_handler(Seq_idx, Handler);
  Seq_idx size() const;
  bool empty() const;

  // Step-level CRUD
  void push_back(Seq_idx, const T_event&);
  void push_back(Seq_idx, Sequence_initializer_list);
  void push_front(Seq_idx, const T_event&);
  void push_front(Seq_idx, Sequence_initializer_list);

  Step_idx insert(Seq_idx, Step_idx, const T_event&);
  Step_idx insert(Seq_idx, Step_idx, T_event&&);

  template <typename... Args> void update(Seq_idx, Step_idx, Args&&... args);
  void replace(Seq_idx, Step_idx, const T_event&);
  void replace(Seq_idx, Step_idx, T_event&&);

  const T_event front(Seq_idx) const;
  const T_event back(Seq_idx) const;
  const T_event at(Seq_idx, Step_idx) const;

  void pop_back(Seq_idx);
  void pop_front(Seq_idx);

  void erase(Seq_idx, Step_idx);

  // Clear all
  void clear();
  void clear(Seq_idx);

private:
  std::mutex transport_mutex_;
  std::mutex crud_mutex_;

  Sequencer_vector sequencers_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_PARALLEL_SEQUENCER_H
