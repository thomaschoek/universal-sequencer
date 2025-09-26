#ifndef MICRO_COMPOSER_CONTROLLABLE_SEQUENCER_H
#define MICRO_COMPOSER_CONTROLLABLE_SEQUENCER_H

#include "sequencable/concept.h"
#include "sequencable/event.h"
#include "sequence/atomic_step_sequence.h"
#include <chrono>
#include <functional>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;

class Controllable_sequencer {
public:
  using clock = std::chrono::steady_clock;
  using time_point = typename clock::time_point;
  using Step_idx = std::size_t;
  using Duration_t = std::chrono::duration<double>;
  using Base_event_t = sequencable::Event;

  using Handler_t = typename std::function<void(Base_event_t&&)>;
  using Sequence_t = typename sequence::Atomic_step_sequence<Base_event_t>;
  using size_type = typename Sequence_t::size_type;
  using Sequence_itr_t = typename Sequence_t::iterator;

  virtual ~Controllable_sequencer() noexcept = default;

  virtual void start(time_point) = 0;
  virtual void stop() = 0;
  virtual bool is_running() const = 0;

  virtual size_type size() const noexcept = 0;

  virtual const Base_event_t* at(Step_idx) const = 0;

  virtual void toggle(Step_idx) = 0;
  virtual void toggle_all() = 0;
};

} // namespace sequencer

} // namespace Micro_composer

#endif
