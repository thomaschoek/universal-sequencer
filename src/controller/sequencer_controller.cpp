#include "controller/sequencer_controller.tpp"

namespace Micro_composer {

namespace controller {

void Multi_sequence_controller::add_seq(Seq_ptr_t&& ptr) {
  Base_t::add_seq(std::move(ptr));
}

void Multi_sequence_controller::select_seq(const Seq_idx idx) {
  if (idx >= Base_t::n_seqs()) {
    throw std::out_of_range(
        "Attempted to select a sequencer at an invalid index");
  }
  seq_idx_ = idx;
}

void Multi_sequence_controller::select_seq_next() {
  if (++seq_idx_ >= Base_t::n_seqs()) {
    seq_idx_ = 0;
  }
}

void Multi_sequence_controller::select_seq_prev() {
  if (--seq_idx_ < 0) {
    seq_idx_ = Base_t::n_seqs() - 1;
  }
}

void Multi_sequence_controller::toggle_seq_current() {
  if (Base_t::get_seq(seq_idx_)->is_running()) {
    Base_t::stop(seq_idx_);
  } else {
    Base_t::start(seq_idx_);
  }
}

void Multi_sequence_controller::drop_seq_current() {
  Base_t::drop_seq(seq_idx_);
  if (seq_idx_ >= Base_t::n_seqs() && Base_t::n_seqs() > 0) {
    seq_idx_ = Base_t::n_seqs() - 1;
  }
}

void Multi_sequence_controller::select_step(const Step_idx idx) {
  if (idx >= Base_t::get_seq(seq_idx_)->size()) {
    throw std::out_of_range("Attempted to select a step at an invalid index");
  }
  step_idx_ = idx;
}

void Multi_sequence_controller::select_step_next() {
  if (++step_idx_ >= Base_t::get_seq(seq_idx_)->size()) {
    step_idx_ = 0;
  }
}

void Multi_sequence_controller::select_step_prev() {
  if (--step_idx_ < 0) {
    step_idx_ = Base_t::get_seq(seq_idx_)->size() - 1;
  }
}

void Multi_sequence_controller::toggle_step_current() {
  Base_t::toggle_step(seq_idx_, step_idx_);
}

void Multi_sequence_controller::drop_step_current() {
  auto step = Base_t::get_seq(seq_idx_)->at(step_idx_);
  Base_t::erase_step<decltype(step)>(seq_idx_, step_idx_);
}

void Multi_sequence_controller::start_all(Seq_time_point common_start_time) {
  Base_t::start_all(common_start_time);
}

void Multi_sequence_controller::stop_all() { Base_t::stop_all(); }

void Multi_sequence_controller::start(const Seq_idx idx,
                                      Seq_time_point start_time) {
  Base_t::start(idx, start_time);
}

void Multi_sequence_controller::stop(const Seq_idx idx) { Base_t::stop(idx); }

} // namespace controller

} // namespace Micro_composer
