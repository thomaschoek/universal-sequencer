#include "matrix_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

void Matrix_sequencer::start(Seq_idx idx, Time_point start_time) {
  std::scoped_lock lck{transport_mutex_};
  sequencers_.at(idx).start(start_time);
}

void Matrix_sequencer::start_all(Time_point common_start_time) {
  // Launch all sequencers asynchronously with the same start time
  std::scoped_lock lck{transport_mutex_};

  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
    futures.emplace_back(
        std::async(std::launch::async, [&sequencer, common_start_time]() {
          sequencer.start(common_start_time);
        }));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

void Matrix_sequencer::stop(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  sequencers_.at(idx).stop();
}

void Matrix_sequencer::stop_all() {
  std::scoped_lock lck{transport_mutex_};
  std::vector<std::future<void>> futures;
  futures.reserve(sequencers_.size());

  for (auto& sequencer : sequencers_) {
    futures.emplace_back(std::async([&sequencer]() { sequencer.stop(); }));
  }

  // Wait for all to complete startup
  for (auto& future : futures) {
    future.wait();
  }
}

// CRUD Operations
/// Sequence-level CRUD

void Matrix_sequencer::add_sequence(Sequence_initializer_list seq,
                                    Handler handler) {
  sequencers_.emplace_back(seq, handler);
}

void Matrix_sequencer::drop_sequence(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  if (idx < sequencers_.size()) {
    sequencers_.erase(sequencers_.begin() + idx);
  }
}

void Matrix_sequencer::set_handler(Seq_idx idx, Handler handler) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).set_handler(handler);
}

void Matrix_sequencer::assign(Seq_idx idx,
                              std::initializer_list<Vector_event> seq) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).assign(seq);
}

Matrix_sequencer::Seq_idx Matrix_sequencer::size() const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.size();
}

bool Matrix_sequencer::empty() const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.empty();
}

const Matrix_sequencer::Sequence& Matrix_sequencer::get(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).steps();
}

//// Step-level CRUD

void Matrix_sequencer::push_back(Seq_idx idx, const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).push_back(event);
}

void Matrix_sequencer::push_back(Seq_idx idx,
                                 Sequence_initializer_list events) {
  std::scoped_lock lck{crud_mutex_};
  for (const auto& event : events) {
    sequencers_.at(idx).push_back(event);
  }
}

void Matrix_sequencer::push_front(Seq_idx idx, const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).push_front(event);
}

void Matrix_sequencer::push_front(Seq_idx idx,
                                  Sequence_initializer_list events) {
  std::scoped_lock lck{crud_mutex_};
  // Insert in reverse order to maintain sequence
  for (auto it = events.end(); it != events.begin();) {
    --it;
    sequencers_.at(idx).push_front(*it);
  }
}

void Matrix_sequencer::pop_back(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).pop_back();
}

void Matrix_sequencer::pop_front(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).pop_front();
}

Matrix_sequencer::Step_iterator
Matrix_sequencer::insert(Seq_idx seq_idx, Sequencer::Step_idx step_idx,
                         const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).insert(step_idx, event);
}

Matrix_sequencer::Step_iterator
Matrix_sequencer::insert(Seq_idx seq_idx, Sequencer::Step_idx step_idx,
                         Vector_event&& event) {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).insert(step_idx, std::move(event));
}

const Matrix_sequencer::Vector_event&
Matrix_sequencer::front(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).front();
}

const Matrix_sequencer::Vector_event&
Matrix_sequencer::back(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).back();
}

const Matrix_sequencer::Vector_event&
Matrix_sequencer::at(Seq_idx seq_idx, Sequencer::Step_idx step_idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).at(step_idx);
}

void Matrix_sequencer::clear() {
  std::scoped_lock lck{crud_mutex_};
  for (auto& sequencer : sequencers_) {
    sequencer.clear();
  }
}

void Matrix_sequencer::clear(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).clear();
}

} // namespace sequencer

} // namespace Micro_composer
