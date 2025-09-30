#include "container/atomic_vector.h"
#include "sequencer/atomic_sequencer.h"
#include "sequencer/parallel_sequencer.h"
#include <stdexcept>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer<Event_t>::start(Seq_idx idx, Time_point start_time) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Parallel_sequencer::start: Index out of range.");
  }
  Base_vector::operator[](idx).start(start_time);
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer<Event_t>::start_all(Time_point start_time) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).start(start_time);
  }
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer<Event_t>::stop(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Parallel_sequencer::stop: Index out of range.");
  }
  Base_vector::operator[](idx).stop();
}

template <sequencable::Sequencable_updatable Event_t>
void Parallel_sequencer<Event_t>::stop_all() {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).stop();
  }
}

template <sequencable::Sequencable_updatable Event_t>
bool Parallel_sequencer<Event_t>::is_running(Seq_idx idx) const {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Parallel_sequencer::is_running: Index out of range.");
  }
  return Base_vector::operator[](idx).is_running();
}

template <sequencable::Sequencable_updatable Event_t>
bool Parallel_sequencer<Event_t>::any_running() const {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    if (Base_vector::operator[](i).is_running()) {
      return true;
    }
  }
  return false;
}

template <sequencable::Sequencable_updatable Event_t>
bool Parallel_sequencer<Event_t>::all_running() const {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  if (count == 0) {
    return false;
  }
  for (Seq_idx i = 0; i < count; ++i) {
    if (!Base_vector::operator[](i).is_running()) {
      return false;
    }
  }
  return true;
}

} // namespace sequencer
} // namespace Micro_composer