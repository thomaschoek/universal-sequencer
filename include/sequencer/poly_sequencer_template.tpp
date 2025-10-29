#include "sequencer/poly_sequencer_template.h"
#include <stdexcept>

namespace Micro_composer {

namespace sequencer {

// Constructors

template <sequencable::Mut_seq_event Event_t>
Poly_sequencer<Event_t>::Poly_sequencer(
    const Handler_factory& factory,
    const std::vector<std::vector<Event_t>>& sequences)
    : handler_factory_{factory} {
  for (size_t i = 0; i < sequences.size(); ++i) {
    Base_vector::push_back(Sequencer_t(handler_factory_(), sequences[i]));
  }
}

template <sequencable::Mut_seq_event T_event>
Poly_sequencer<T_event>::Poly_sequencer(
    const std::vector<Handler>& handlers,
    const std::vector<std::vector<T_event>>& sequences) {
  if (handlers.size() != sequences.size()) {
    throw std::invalid_argument(
        "Poly_sequencer: number of handlers must match number of sequences");
  }
  for (size_t i = 0; i < sequences.size(); ++i) {
    Base_vector::push_back(Sequencer_t(handlers[i], sequences[i]));
  }
}

template <sequencable::Mut_seq_event T_event>
Poly_sequencer<T_event>::Poly_sequencer(
    const std::vector<Handler>& handlers,
    std::vector<std::vector<T_event>>&& sequences) {
  if (handlers.size() != sequences.size()) {
    throw std::invalid_argument(
        "Poly_sequencer: number of handlers must match number of sequences");
  }
  for (size_t i = 0; i < sequences.size(); ++i) {
    Base_vector::push_back(Sequencer_t(handlers[i], std::move(sequences[i])));
  }
}

template <sequencable::Mut_seq_event T_event>
Poly_sequencer<T_event>::Poly_sequencer(std::vector<Sequencer_t>&& sequencers)
    : Base_vector(std::move(sequencers)) {}

// Transport control

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::start(Seq_idx idx, Time_point start_time,
                                    bool repeat) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::start: Index out of range.");
  }
  Base_vector::operator[](idx).start(start_time, repeat);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::start(Time_point start_time, bool repeat) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).start(start_time, repeat);
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::pause(Seq_idx idx, Time_point pause_time) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::pause: Index out of range.");
  }
  Base_vector::operator[](idx).pause(pause_time);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::pause(Time_point pause_time) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).pause(pause_time);
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::stop(Seq_idx idx, Time_point stop_time,
                                   size_t stop_pos) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::stop: Index out of range.");
  }
  Base_vector::operator[](idx).stop(stop_time, stop_pos);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::stop(Time_point stop_time, size_t stop_pos) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).stop(stop_time, stop_pos);
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::set_pos(Seq_idx idx, size_t pos) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::set_pos: Index out of range.");
  }
  Base_vector::operator[](idx).set_pos(pos);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::set_common_pos(size_t pos) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).set_pos(pos);
  }
}

template <sequencable::Mut_seq_event T_event>
bool Poly_sequencer<T_event>::is_scheduling(Seq_idx idx) const {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::is_scheduling: Index out of range.");
  }
  return Base_vector::operator[](idx).is_scheduling();
}

template <sequencable::Mut_seq_event T_event>
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

template <sequencable::Mut_seq_event T_event>
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

template <sequencable::Mut_seq_event Event_t>
void Poly_sequencer<Event_t>::set_handler_factory(
    const Handler_factory& factory) {
  std::scoped_lock lck{Base_vector::get_lock()};
  handler_factory_ = factory;
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::set_handler(Seq_idx idx, const Handler& handler) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::set_handler: Index out of range.");
  }
  Base_vector::operator[](idx).set_handler(handler);
}

template <sequencable::Mut_seq_event T_event>
template <typename Handler_container>
void Poly_sequencer<T_event>::set_handlers(const Handler_container& handlers) {
  std::scoped_lock lck{transport_mutex_};
  auto seq_count = Base_vector::size();
  Seq_idx idx = 0;
  for (const auto& handler : handlers) {
    if (idx >= seq_count) {
      break;
    }
    Base_vector::operator[](idx).set_handler(handler);
    ++idx;
  }
}

// Event modification methods

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::update(Seq_idx seq,
                                     typename Sequencer_t::Size_type pos,
                                     const T_event& event) {
  std::scoped_lock lck{transport_mutex_};
  if (seq >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::update: Sequencer index out of range.");
  }
  Base_vector::operator[](seq).update(pos, event);
}

template <sequencable::Mut_seq_event T_event>
template <typename... Args>
void Poly_sequencer<T_event>::update(Seq_idx seq,
                                     typename Sequencer_t::Size_type pos,
                                     Args&&... args) {
  std::scoped_lock lck{transport_mutex_};
  if (seq >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::update: Sequencer index out of range.");
  }
  Base_vector::operator[](seq).update(pos, std::forward<Args>(args)...);
}

// Toggling events on/off
template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::enable() {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).enable();
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::enable(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::enable: Index out of range.");
  }
  Base_vector::operator[](idx).enable();
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::enable(Seq_idx i_seq, Event_idx i_event) {
  std::scoped_lock lck{transport_mutex_};
  if (i_seq >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::enable: Index out of range.");
  }
  Base_vector::operator[](i_seq).enable(i_event);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::disable() {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).disable();
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::disable(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::disable: Index out of range.");
  }
  Base_vector::operator[](idx).disable();
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::disable(Seq_idx i_seq, Event_idx i_event) {
  std::scoped_lock lck{transport_mutex_};
  if (i_seq >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::disable: Index out of range.");
  }
  Base_vector::operator[](i_seq).disable(i_event);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::toggle() {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).toggle();
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::toggle(Seq_idx idx) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::toggle: Index out of range.");
  }
  Base_vector::operator[](idx).toggle();
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::toggle(Seq_idx i_seq, Event_idx i_event) {
  std::scoped_lock lck{transport_mutex_};
  if (i_seq >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::toggle: Index out of range.");
  }
  Base_vector::operator[](i_seq).toggle(i_event);
}

// Adjusting sequencer tempo

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::adjust_durations(Seq_idx idx, Duration delta) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::adjust_durations: Index out of range.");
  }
  Base_vector::operator[](idx).adjust_durations(delta);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::adjust_durations(Duration delta) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).adjust_durations(delta);
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::multiply_durations(Seq_idx idx, double factor) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::multiply_durations: Index out of range.");
  }
  Base_vector::operator[](idx).multiply_durations(factor);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::multiply_durations(double factor) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).multiply_durations(factor);
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::for_each(
    Seq_idx idx, const std::function<void(T_event&)>& func) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::for_each: Index out of range.");
  }
  Base_vector::operator[](idx).mutate([&func](T_event&& evt) {
    func(evt);
    return std::move(evt);
  });
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::for_each(
    const std::function<void(T_event&)>& func) {
  std::scoped_lock lck{transport_mutex_};
  auto count = Base_vector::size();
  for (Seq_idx i = 0; i < count; ++i) {
    Base_vector::operator[](i).mutate([&func](T_event&& evt) {
      func(evt);
      return std::move(evt);
    });
  }
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::mutate(
    Seq_idx seq, Event_idx pos, const typename Sequencer_t::Mutator& mutator) {
  std::scoped_lock lck{transport_mutex_};
  if (seq >= Base_vector::size()) {
    throw std::out_of_range(
        "Poly_sequencer::mutate: Sequence index out of range.");
  }
  Base_vector::operator[](seq).mutate(pos, mutator);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::replace(Seq_idx idx,
                                      typename Sequencer_t::Size_type pos,
                                      const T_event& event) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::replace: Index out of range.");
  }
  Base_vector::operator[](idx).replace(pos, event);
}

template <sequencable::Mut_seq_event T_event>
void Poly_sequencer<T_event>::replace(Seq_idx idx,
                                      typename Sequencer_t::Size_type start,
                                      const std::vector<T_event>& events) {
  std::scoped_lock lck{transport_mutex_};
  if (idx >= Base_vector::size()) {
    throw std::out_of_range("Poly_sequencer::replace: Index out of range.");
  }
  Base_vector::operator[](idx).replace(start, events);
}

} // namespace sequencer
} // namespace Micro_composer
