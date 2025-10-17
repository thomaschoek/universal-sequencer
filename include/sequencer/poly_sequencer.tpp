#include "sequencer/poly_sequencer.h"
#include <future>
#include <stdexcept>

namespace Micro_composer {

namespace sequencer {

// Constructors

template <Has_duration T_event>
Poly_sequencer<T_event>::Poly_sequencer(
    const std::vector<std::vector<T_event>>& sequences) {
  for (const auto& seq : sequences) {
    Base_vector::push_back(Sequencer_t(seq));
  }
}

template <Has_duration T_event>
Poly_sequencer<T_event>::Poly_sequencer(
    std::vector<std::vector<T_event>>&& sequences) {
  for (auto& seq : sequences) {
    Base_vector::push_back(Sequencer_t(std::move(seq)));
  }
}

template <Has_duration T_event>
Poly_sequencer<T_event>::Poly_sequencer(
    std::vector<Sequencer_t>&& sequencers)
    : Base_vector(std::move(sequencers)) {}

// Transport control

template <Has_duration T_event>
void Poly_sequencer<T_event>::start(Seq_idx idx, Time_point start_time,
                                     bool repeat) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::start: Index out of range.");
  }
  Base_vector::operator[](idx).start(start_time, repeat);
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::start_all(Time_point start_time, bool repeat) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    std::ignore = std::async([this, start_time, repeat, i]() {
      Base_vector::operator[](i).start(start_time, repeat);
    });
  }
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::pause(Seq_idx idx, Time_point pause_time) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::pause: Index out of range.");
  }
  Base_vector::operator[](idx).pause(pause_time);
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::pause_all(Time_point pause_time) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).pause(pause_time);
  }
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::reset(Seq_idx idx, Time_point reset_time,
                                     size_t reset_pos) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::reset: Index out of range.");
  }
  Base_vector::operator[](idx).reset(reset_time, reset_pos);
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::reset_all(Time_point reset_time,
                                         size_t reset_pos) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).reset(reset_time, reset_pos);
  }
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::set_next(Seq_idx idx, size_t pos) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::set_next: Index out of range.");
  }
  Base_vector::operator[](idx).set_next(pos);
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::set_next_all(size_t pos) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).set_next(pos);
  }
}

template <Has_duration T_event>
bool Poly_sequencer<T_event>::is_scheduling(Seq_idx idx) const {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::is_scheduling: Index out of range.");
  }
  return Base_vector::operator[](idx).is_scheduling();
}

template <Has_duration T_event>
bool Poly_sequencer<T_event>::any_scheduling() const {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    if (Base_vector::operator[](i).is_scheduling()) {
      return true;
    }
  }
  return false;
}

template <Has_duration T_event>
bool Poly_sequencer<T_event>::all_scheduling() const {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  if (count == 0) {
    return false;
  }
  for (Seq_idx i = 0; i < count; ++i) {
    if (!Base_vector::operator[](i).is_scheduling()) {
      return false;
    }
  }
  return true;
}

// Handler management

template <Has_duration T_event>
void Poly_sequencer<T_event>::listen(Seq_idx idx, Event_handler handler) {
  // Note: listen() blocks until sequencer stops, so we don't lock here
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::listen: Index out of range.");
  }
  Base_vector::operator[](idx).listen(handler);
}

template <Has_duration T_event>
void Poly_sequencer<T_event>::set_handler(Seq_idx idx, Event_handler handler) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Poly_sequencer::set_handler: Index out of range.");
  }
  // Note: Sequencer doesn't have set_handler method yet,
  // so we'll need to store handlers separately or modify Sequencer
  // For now, this is a placeholder
}

template <Has_duration T_event>
template <typename Handler_container>
void Poly_sequencer<T_event>::set_handlers(
    const Handler_container& handlers) {
  std::scoped_lock lck{transport_mutex_};
  auto seq_count = Base_vector::size();
  Seq_idx idx = 0;
  for (const auto& handler : handlers) {
    if (idx >= seq_count) {
      break; // Don't throw, just stop setting handlers
    }
    // Placeholder - needs Sequencer to have set_handler method
    ++idx;
  }
}

} // namespace sequencer
} // namespace Micro_composer
