#ifndef MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
#define MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H

#include "abstract.h"
#include "sequencable/concept.h"
#include "sequence/atomic_ring_deque.h"
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace sequencer {

using sequencable::Sequencable;
using sequence::Atomic_ring_deque;

template <Sequencable Event_t>
class Atomic_sequencer : public abstract::Sequencer {
public:
  using Sequence = Atomic_ring_deque<Event_t>;
  using Handler = std::function<void(Event_t&&)>;
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Step_idx = Sequence::size_type;
  using Step_iterator = Sequence::iterator;
  using Initializer_list = std::initializer_list<Event_t>;

  Atomic_sequencer() = default;
  Atomic_sequencer(const Atomic_sequencer&);
  Atomic_sequencer& operator=(const Atomic_sequencer&);
  Atomic_sequencer(Atomic_sequencer&&) noexcept;

  explicit Atomic_sequencer(Handler);
  Atomic_sequencer(Initializer_list, Handler);

  bool is_running() const override;
  void start(Time_point start_time = Clock::now()) override;
  void stop() override;

  void set_handler(const Handler);
  void assign(Initializer_list);

  // Thread-safe operations
  void push_back(const Event_t&);
  void push_back(Event_t&&);
  void push_front(const Event_t&);
  void push_front(Event_t&&);

  Step_iterator insert(Step_idx, const Event_t&);
  Step_iterator insert(Step_idx, Event_t&&);

  void pop_back();
  void pop_front();

  void clear();
  Step_idx size() const;
  bool empty() const;

  Event_t& front();
  const Event_t& front() const;
  Event_t& back();
  const Event_t& back() const;

  Event_t& at(Step_idx);
  const Event_t& at(Step_idx) const;
  Event_t& operator[](Step_idx);
  const Event_t& operator[](Step_idx) const;

  const Sequence& steps() const;

private:
  void run(std::stop_token st, Time_point start_time);
  Sequence steps_;
  std::jthread runner_thread_;
  Handler handler_ = [](Event_t&&) {};

  std::mutex transport_mutex_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif // MICRO_COMPOSER_ATOMIC_EVENT_SEQUENCER_H
