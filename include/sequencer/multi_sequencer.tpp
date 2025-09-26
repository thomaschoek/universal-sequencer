#include "sequencer/atomic_sequencer.tpp"
#include "sequencer/multi_sequencer.h"
#include <cassert>
#include <future>

namespace Micro_composer {

namespace sequencer {

// CRUD
// Sequences
template <Sequencable Event_t>
void Multi_sequencer::add_seq(std::function<void(Event_t&&)> handler,
                              sequence::Atomic_step_sequence<Event_t>&& seq) {
  sequences_.emplace_back(
      std::make_unique<Atomic_sequencer<Event_t>>(handler, std::move(seq)));
}

void Multi_sequencer::add_seq(Seq_ptr_t&& seqr) {
  sequences_.emplace_back(std::move(seqr));
}

inline const Multi_sequencer::Sequence_vec::size_type
Multi_sequencer::n_seqs() const noexcept {
  return sequences_.size();
}

const Multi_sequencer::Seq_ptr_t&
Multi_sequencer::get_seq(const Seq_idx idx) const {
  return sequences_.at(idx);
}

const Multi_sequencer::Sequence_vec&
Multi_sequencer::get_all_seqs() const noexcept {
  return sequences_;
}

void Multi_sequencer::drop_seq(const Seq_idx idx) {
  if (idx >= sequences_.size()) {
    throw std::out_of_range(
        "Attempted to remove a sequencer at an invalid index");
  }
  sequences_.erase(sequences_.cbegin() + idx);
}

// Steps

template <Sequencable Event_t>
void Multi_sequencer::push_step_back(const Seq_idx idx, Event_t&& event) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->push_back(std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::push_step_front(const Seq_idx idx, Event_t&& event) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->push_front(std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::pop_step_back(const Seq_idx idx) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->pop_back();
}

template <Sequencable Event_t>
void Multi_sequencer::pop_step_front(const Seq_idx idx) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->pop_front();
}

template <Sequencable Event_t>
void Multi_sequencer::insert_step(const Seq_idx idx, const Step_idx step_index,
                                  Event_t&& event) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->insert(step_index, std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::set_step(const Seq_idx idx, const Step_idx step_index,
                               Event_t&& event) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->update(step_index, std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::erase_step(const Seq_idx idx, const Step_idx step_index) {
  auto seqr = static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequences_.at(idx).get()) ==
         seqr);

  seqr->erase(step_index);
}

template <Sequencable Event_t>
void Multi_sequencer::erase_steps(const Seq_idx seq_idx, const Step_idx first,
                                  const Step_idx last) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequences_.at(seq_idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(
             sequences_.at(seq_idx).get()) == seqr);

  auto begin_itr = seqr->cbegin();

  seqr->erase(begin_itr + first, begin_itr + last);
}

// Control
void Multi_sequencer::start_all(const Seq_time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(sequences_.size());

  for (auto& sequencer_ : sequences_) {
    futures.emplace_back(
        std::async(std::launch::async, [&sequencer_, common_start_time]() {
          sequencer_->start(common_start_time);
        }));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

void Multi_sequencer::stop_all() {
  std::vector<std::future<void>> futures;
  futures.reserve(sequences_.size());

  for (auto& sequencer : sequences_) {
    futures.emplace_back(std::async(
        std::launch::async, &Controllable_sequencer::stop, sequencer.get()));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

inline void Multi_sequencer::start(const Seq_idx idx,
                                   const Seq_time_point start_time) {
  sequences_.at(idx)->start(start_time);
}

inline void Multi_sequencer::stop(const Seq_idx idx) {
  sequences_.at(idx)->stop();
}

} // namespace sequencer

} // namespace Micro_composer
