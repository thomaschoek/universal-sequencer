#ifndef MICRO_COMPOSER_POLY_SEQUENCER_H
#define MICRO_COMPOSER_POLY_SEQUENCER_H

#include "container/atomic_vector.h"
#include "sequencer/sequencer.h"
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace sequencer {

template <Has_duration T_event>
class Poly_sequencer
    : public container::Atomic_vector<Sequencer<T_event>> {
public:
  using Sequencer_t = Sequencer<T_event>;
  using Base_vector = container::Atomic_vector<Sequencer_t>;
  using Seq_idx = typename Base_vector::Index;
  using Event_handler = typename Sequencer_t::Event_handler;
  using Clock = typename Sequencer_t::Clock;
  using Time_point = typename Sequencer_t::Time_point;
  using Duration = typename Sequencer_t::Duration;
  using Data_init_list = typename Sequencer_t::Data_init_list;

  // Constructors
  Poly_sequencer() = default;
  Poly_sequencer(const Poly_sequencer&) = delete;
  Poly_sequencer& operator=(const Poly_sequencer&) = delete;
  Poly_sequencer(Poly_sequencer&&) noexcept = default;
  ~Poly_sequencer() = default;

  Poly_sequencer(const std::vector<std::vector<T_event>>&);
  Poly_sequencer(std::vector<std::vector<T_event>>&&);
  Poly_sequencer(std::vector<Sequencer_t>&&);

  // Synchronized transport control
  void start(Seq_idx, Time_point start_time = Clock::now(), bool repeat = false);
  void start_all(Time_point start_time = Clock::now(), bool repeat = false);

  void pause(Seq_idx, Time_point pause_time = Clock::now());
  void pause_all(Time_point pause_time = Clock::now());

  void reset(Seq_idx, Time_point reset_time = Clock::now(), size_t reset_pos = 0);
  void reset_all(Time_point reset_time = Clock::now(), size_t reset_pos = 0);

  void set_next(Seq_idx, size_t pos = 0);
  void set_next_all(size_t pos = 0);

  bool is_scheduling(Seq_idx) const;
  bool any_scheduling() const;
  bool all_scheduling() const;

  // Handler management
  void listen(Seq_idx, Event_handler);
  void set_handler(Seq_idx, Event_handler);
  template <typename Handler_container>
  void set_handlers(const Handler_container&);

protected:
  mutable std::mutex transport_mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/poly_sequencer.tpp"

#endif // MICRO_COMPOSER_POLY_SEQUENCER_H
