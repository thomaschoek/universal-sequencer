#include "matrix_sequencer.h"
#include <future>

namespace Micro_composer {

namespace sequencer {

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::start(Seq_idx idx,
                                             Time_point start_time) {
  std::scoped_lock lck{transport_mutex_};
  sequencers_.at(idx).start(start_time);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::start_all(Time_point common_start_time) {
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

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::stop(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  sequencers_.at(idx).stop();
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::stop_all() {
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

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::add_sequence(
    Sequence_initializer_list seq, Handler handler) {
  sequencers_.emplace_back(seq, handler);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::drop_sequence(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  if (idx < sequencers_.size()) {
    sequencers_.erase(sequencers_.begin() + idx);
  }
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::set_handler(Seq_idx idx,
                                                   Handler handler) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).set_handler(handler);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::assign(
    Seq_idx idx, std::initializer_list<Vector_event> seq) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).assign(seq);
}

template <typename T_event_params>
Matrix_sequencer<T_event_params>::Seq_idx
Matrix_sequencer<T_event_params>::size() const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.size();
}

template <typename T_event_params>
bool Matrix_sequencer<T_event_params>::empty() const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.empty();
}

template <typename T_event_params>
const Matrix_sequencer<T_event_params>::Steps&
Matrix_sequencer<T_event_params>::get(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).steps();
}

//// Step-level CRUD

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::push_back(Seq_idx idx,
                                                 const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).push_back(event);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::push_back(
    Seq_idx idx, Sequence_initializer_list events) {
  std::scoped_lock lck{crud_mutex_};
  for (const auto& event : events) {
    sequencers_.at(idx).push_back(event);
  }
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::push_front(Seq_idx idx,
                                                  const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).push_front(event);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::push_front(
    Seq_idx idx, Sequence_initializer_list events) {
  std::scoped_lock lck{crud_mutex_};
  // Insert in reverse order to maintain sequence
  for (auto it = events.end(); it != events.begin();) {
    --it;
    sequencers_.at(idx).push_front(*it);
  }
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::pop_back(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).pop_back();
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::pop_front(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).pop_front();
}

template <typename T_event_params>
Matrix_sequencer<T_event_params>::Step_idx
Matrix_sequencer<T_event_params>::insert(Seq_idx seq_idx,
                                         Sequencer::Step_idx step_idx,
                                         const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).insert(step_idx, event);
}

template <typename T_event_params>
Matrix_sequencer<T_event_params>::Step_idx
Matrix_sequencer<T_event_params>::insert(Seq_idx seq_idx,
                                         Sequencer::Step_idx step_idx,
                                         Vector_event&& event) {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).insert(step_idx, std::move(event));
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::update(Seq_idx seq_idx,
                                              Sequencer::Step_idx step_idx,
                                              size_t param_idx,
                                              T_event_params&& value) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(seq_idx).update(step_idx, param_idx, std::forward(value));
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::replace(Seq_idx seq_idx,
                                               Sequencer::Step_idx step_idx,
                                               const Vector_event& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(seq_idx).replace(step_idx, event);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::replace(Seq_idx seq_idx,
                                               Sequencer::Step_idx step_idx,
                                               Vector_event&& event) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(seq_idx).replace(step_idx, std::forward(event));
}

template <typename T_event_params>
const Matrix_sequencer<T_event_params>::Vector_event
Matrix_sequencer<T_event_params>::front(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).front();
}

template <typename T_event_params>
const Matrix_sequencer<T_event_params>::Vector_event
Matrix_sequencer<T_event_params>::back(Seq_idx idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(idx).back();
}

template <typename T_event_params>
const Matrix_sequencer<T_event_params>::Vector_event
Matrix_sequencer<T_event_params>::at(Seq_idx seq_idx,
                                     Sequencer::Step_idx step_idx) const {
  std::scoped_lock lck{crud_mutex_};
  return sequencers_.at(seq_idx).at(step_idx);
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::clear() {
  std::scoped_lock lck{crud_mutex_};
  for (auto& sequencer : sequencers_) {
    sequencer.clear();
  }
}

template <typename T_event_params>
void Matrix_sequencer<T_event_params>::clear(Seq_idx idx) {
  std::scoped_lock lck{crud_mutex_};
  sequencers_.at(idx).clear();
}

} // namespace sequencer

} // namespace Micro_composer
