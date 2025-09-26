#include "sequencer_controller.h"

namespace Micro_composer {

namespace controller {

template <Sequencable Event_t>
void Multi_sequence_controller::add_seq(
    std::function<void(Event_t&&)> handler,
    sequence::Atomic_step_sequence<Event_t>&& seq) {
  Base_t::add_seq<Event_t>(handler, std::move(seq));
}

template <Sequencable Event_t>
void Multi_sequence_controller::push_step_back(const Seq_idx idx,
                                               Event_t&& evt) {
  Base_t::push_step_back<Event_t>(idx, std::forward<Event_t>(evt));
}

template <Sequencable Event_t>
void Multi_sequence_controller::push_step_front(const Seq_idx, Event_t&& evt) {
  Base_t::push_step_front(std::forward<Event_t>(evt));
}

template <Sequencable Event_t>
void Multi_sequence_controller::pop_step_back(const Seq_idx idx) {
  Base_t::pop_step_back<Event_t>(idx);
}

template <Sequencable Event_t>
inline void Multi_sequence_controller::pop_step_front(const Seq_idx idx) {
  Base_t::pop_step_front<Event_t>(idx);
}

} // namespace controller

} // namespace Micro_composer
