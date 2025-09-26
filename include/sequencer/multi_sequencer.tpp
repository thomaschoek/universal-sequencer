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
  sequencers_.emplace_back(
      std::make_unique<Atomic_sequencer<Event_t>>(handler, std::move(seq)));
}

void Multi_sequencer::add_seq(std::unique_ptr<Sequencer_base>&& seqr) {
  sequencers_.emplace_back(std::move(seqr));
}

void Multi_sequencer::drop_seq(Seqr_idx idx) {
  if (idx >= sequencers_.size()) {
    throw std::out_of_range(
        "Attempted to remove a sequencer at an invalid index");
  }
  sequencers_.erase(sequencers_.begin() + idx);
}

// Steps

template <Sequencable Event_t>
void Multi_sequencer::push_back(Seqr_idx idx, Event_t&& event) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->push_back(std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::push_front(Seqr_idx idx, Event_t&& event) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->push_front(std::move(event));
}

template <Sequencable Event_t> void Multi_sequencer::pop_back(Seqr_idx idx) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->pop_back();
}

template <Sequencable Event_t> void Multi_sequencer::pop_front(Seqr_idx idx) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->pop_front();
}

template <Sequencable Event_t>
void Multi_sequencer::insert(Seqr_idx idx, Step_idx step_index,
                             Event_t&& event) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->insert(step_index, std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::update(Seqr_idx idx, Step_idx step_index,
                             Event_t&& event) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->update(step_index, std::move(event));
}

template <Sequencable Event_t>
void Multi_sequencer::erase(Seqr_idx idx, Step_idx step_index) {
  auto seqr =
      static_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get());

  assert(dynamic_cast<Atomic_sequencer<Event_t>*>(sequencers_.at(idx).get()) ==
         seqr);

  seqr->erase(step_index);
}

// Control
void Multi_sequencer::start_all(Sequencer_time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer_ : sequencers_) {
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
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
    futures.emplace_back(
        std::async(std::launch::async, &Sequencer_base::stop, sequencer.get()));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

inline void Multi_sequencer::start(Seqr_idx idx,
                                   Sequencer_time_point start_time) {
  sequencers_.at(idx)->start(start_time);
}

inline void Multi_sequencer::stop(Seqr_idx idx) { sequencers_.at(idx)->stop(); }

} // namespace sequencer

} // namespace Micro_composer
