#ifndef MICRO_COMPOSER_POLY_SEQUENCER_H
#define MICRO_COMPOSER_POLY_SEQUENCER_H

#include "container/atomic_vector.h"
#include "sequencer/sequencer_template.h"
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace sequencer {

template <sequencable::Mut_seq_event Event_t>
class Poly_sequencer : public container::Atomic_vector<Sequencer<Event_t>> {
public:
  using Sequencer_t = Sequencer<Event_t>;
  using Base_vector = container::Atomic_vector<Sequencer_t>;
  using Seq_idx = Base_vector::Size_type;
  using Handler = Sequencer_t::Handler;
  using Handler_factory = std::function<Handler()>;
  using Clock = Sequencer_t::Clock;
  using Time_point = Sequencer_t::Time_point;
  using Duration = Sequencer_t::Duration;
  using Events_initializer = Sequencer_t::Events_initializer;

  // Constructors
  Poly_sequencer() = default;
  Poly_sequencer(const Poly_sequencer&) = delete;
  Poly_sequencer& operator=(const Poly_sequencer&) = delete;
  Poly_sequencer(Poly_sequencer&&) noexcept = default;
  ~Poly_sequencer() = default;

  // Construct from sequences with corresponding handlers
  explicit Poly_sequencer(const Handler_factory&,
                          const std::vector<std::vector<Event_t>>& = {});
  Poly_sequencer(const std::vector<Handler>& handlers,
                 const std::vector<std::vector<Event_t>>& sequences);
  Poly_sequencer(const std::vector<Handler>& handlers,
                 std::vector<std::vector<Event_t>>&& sequences);
  Poly_sequencer(std::vector<Sequencer_t>&&);

  // Synchronized transport control
  void start(Seq_idx, Time_point start_time = Clock::now(),
             bool repeat = false);
  void start_all(Time_point start_time = Clock::now(), bool repeat = false);

  void pause(Seq_idx, Time_point pause_time = Clock::now());
  void pause_all(Time_point pause_time = Clock::now());

  void stop(Seq_idx, Time_point stop_time = Clock::now(), size_t stop_pos = 0);
  void stop_all(Time_point stop_time = Clock::now(), size_t stop_pos = 0);

  void set_pos(Seq_idx, size_t pos = 0);
  void set_pos_all(size_t pos = 0);

  bool is_scheduling(Seq_idx) const;
  bool any_scheduling() const;
  bool all_scheduling() const;

  // Handler management
  void set_handler_factory(const Handler_factory&);
  void set_handler(Seq_idx, const Handler&);
  template <typename Handler_container>
  void set_handlers(const Handler_container&);

  // Event modification methods
  void update(Seq_idx seq, typename Sequencer_t::Size_type pos,
              const Event_t& event);
  template <typename... Args>
  void update(Seq_idx seq, typename Sequencer_t::Size_type pos, Args&&... args);

  void adjust_durations(Seq_idx, Duration delta);
  void adjust_durations_all(Duration delta);

  void multiply_durations(Seq_idx, double factor);
  void multiply_durations_all(double factor);

  void for_each(Seq_idx, const std::function<void(Event_t&)>&);
  void for_each_all(const std::function<void(Event_t&)>&);

  void replace(Seq_idx, typename Sequencer_t::Size_type pos,
               const Event_t& event);
  void replace(Seq_idx, typename Sequencer_t::Size_type start,
               const std::vector<Event_t>& events);

protected:
  mutable std::mutex transport_mutex_;

  Handler_factory handler_factory_;
};

} // namespace sequencer
} // namespace Micro_composer

#include "sequencer/poly_sequencer_template.tpp"

#endif // MICRO_COMPOSER_POLY_SEQUENCER_H
