#ifndef MICRO_COMPOSER_MATRIX_EVENT_H
#define MICRO_COMPOSER_MATRIX_EVENT_H

#include "sequencable/concepts.h"
#include "sequencable/mutable_event.h"

namespace Micro_composer {
namespace sequencable {

template <Mut_seq_event Event_t> struct Matrix_event : public Mutable_event {
  std::vector<std::vector<Event_t>> data;

  // Default constructor required by Seq_event concept
  Matrix_event() = default;

  Matrix_event(Duration dur, Time_point time,
               const std::vector<std::vector<Event_t>>& d)
      : Mutable_event{dur, time}, data(d) {}

  void update_data(const std::vector<std::vector<Event_t>>& new_data) {
    data = new_data;
  }

  void update(const Matrix_event& other) {
    duration = other.duration;
    scheduled_time = other.scheduled_time;
    data = other.data;
  }

  void update(Duration dur, Time_point time,
              const std::vector<std::vector<Event_t>>& d) {
    duration = dur;
    scheduled_time = time;
    data = d;
  }
};

} // namespace sequencable
} // namespace Micro_composer

#endif
