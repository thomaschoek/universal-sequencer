#include "thread_pool.h"
#include <cassert>
#ifndef NDEBUG
#include <iostream>
#include <syncstream>
#endif
#include <stdexcept>

namespace Micro_composer {

namespace sequencer {

namespace thread_pool {

#ifndef NDEBUG

long get_timestamp_ms() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

inline void debug_msg(std::string msg, std::ostream& stream = std::cerr) {
#ifndef NDEBUG

  std::osyncstream(stream) << get_timestamp_ms() << " [THREAD_POOL] thread "
                           << std::to_string(std::hash<std::thread::id>{}(
                                  std::this_thread::get_id()))
                           << ": " << msg << std::endl
                           << std::flush;
#endif
}

#endif

// Constructors

template <sequencable::Sequencable T_event>
Thread_pool<T_event>::Thread_pool(Task event_handler,
                                  Size_type initial_n_threads)
    : handler_{event_handler} {
  if (handler_ == nullptr) {
    throw std::invalid_argument("Thread_pool: event_handler cannot be null");
  }
  std::scoped_lock lck{workers_mutex_};
  for (Size_type i = 0; i < initial_n_threads; ++i) {
    workers_.push_back(std::make_unique<std::jthread>(worker()));
  }
}

// Destructor
template <sequencable::Sequencable T_event>
Thread_pool<T_event>::~Thread_pool() {
  debug_msg("MAIN THREAD Destructor starting");
  std::scoped_lock lck{workers_mutex_, events_mutex_};
  stop_workers();
  debug_msg("RETURNED FROM stop_workers()");
}

template <sequencable::Sequencable T_event>
Thread_pool<T_event>::Thread_pool(Thread_pool&& other) noexcept {
  {
    std::scoped_lock lck{other.events_mutex_};
    events_ = std::move(other.events_);
  }
  std::scoped_lock lck{other.workers_mutex_, cv_mutex_};
  other.stop_workers();
  assert(other.handler_ != nullptr &&
         "Thread_pool move constructor: other.handler_ is null");
  handler_ = std::move(other.handler_);
  workers_ = std::move(other.workers_);
}

template <sequencable::Sequencable T_event>
Thread_pool<T_event>&
Thread_pool<T_event>::operator=(Thread_pool&& other) noexcept {
  Thread_pool(std::move(other));
  return *this;
}

// Public

template <sequencable::Sequencable T_event>
void Thread_pool<T_event>::set_handler(const Task& t) {
  if (t == nullptr) {
    throw std::invalid_argument("Thread_pool: handler cannot be null");
  }
  std::scoped_lock lock_cv{cv_mutex_};
  std::scoped_lock lock_workers{workers_mutex_};
  while (workers_idle_.load(std::memory_order_acquire) < workers_.size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  handler_ = t;
}

template <sequencable::Sequencable T_event>
void Thread_pool<T_event>::submit(T_event&& event) {
  push_event(std::forward<T_event>(event));
  std::scoped_lock lck{workers_mutex_};
  if (workers_idle_.load(std::memory_order_acquire) == 0) {
    workers_.push_back(std::make_unique<std::jthread>(worker()));
  }
  cv_.notify_one();
  std::scoped_lock cv_lck{cv_mutex_};
  const Size_type workers_idle = workers_idle_.load(std::memory_order_acquire);
  if (workers_idle > 1 && workers_idle > std::thread::hardware_concurrency()) {
    debug_msg("Too many idle workers (" + std::to_string(workers_idle) +
              "), removing one worker thread");
    workers_.back()->request_stop();
    workers_.pop_back();
  }
}

// Private
template <sequencable::Sequencable T_event>
void Thread_pool<T_event>::push_event(T_event&& evt) {
  std::scoped_lock lck{events_mutex_};
  events_.emplace_back(std::make_unique<T_event>(std::move(evt)));
  new_events_.fetch_add(1, std::memory_order_acq_rel);
}

template <sequencable::Sequencable T_event>
inline T_event Thread_pool<T_event>::pop_event() {
  debug_msg("pop_event()");
  std::scoped_lock lck{events_mutex_};
  if (events_.empty()) {
    throw std::runtime_error(
        "Thread_pool::pop_event() called while events_ is empty!");
  }
  T_event event;
  event = std::move(*events_.front());
  events_.pop_front();
  new_events_.fetch_sub(1, std::memory_order_acq_rel);
  return event;
}

template <sequencable::Sequencable T_event>
std::jthread Thread_pool<T_event>::worker() {
  return std::jthread([this](std::stop_token st) {
    debug_msg("LAUNCHED NEW THREAD");
    while (!st.stop_requested()) {
      workers_idle_.fetch_add(1, std::memory_order_acq_rel);
      {
        debug_msg("ACQUIRING cv_mutex_");
        std::unique_lock lck{cv_mutex_};
        while (new_events_.load(std::memory_order_acquire) == 0) {
          debug_msg("new_events_.load() == 0 && !stop_requested()");
          debug_msg("WAITING on cv_");
          cv_.wait(lck);
          debug_msg("NOTIFIED");
          if (st.stop_requested()) {
            debug_msg("STOP REQUESTED - RETURNING");
            workers_idle_.fetch_sub(1, std::memory_order_acq_rel);
            return;
          }
        }
        workers_idle_.fetch_sub(1, std::memory_order_acq_rel);
      }
      if (st.stop_requested()) {
        debug_msg("STOP REQUESTED - RETURNING");
        return;
      }
      T_event event;
      try {
        debug_msg("ABOUT TO POP ZHE EVENT LOL YES!");
        event = pop_event();
      } catch (const std::exception& e) {
        debug_msg(std::string("EXCEPTION in pop_event(): ") + e.what());
        continue;
      }
      const Time_point& scheduled_time = event.scheduled_time;
      debug_msg(
          "POPPED EVENT with scheduled_time " +
          std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                             scheduled_time.time_since_epoch())
                             .count()) +
          " ms");
      std::this_thread::sleep_until(event.scheduled_time - spin_duration_);
      while (Clock::now() < event.scheduled_time) {
        if (st.stop_requested()) {
          return;
        }
      }
      debug_msg("CALLING HANDLER");
      handler_(std::forward<T_event>(event));
      debug_msg("RETURNED FROM HANDLER");
    }
  });
}

template <sequencable::Sequencable T_event>
void Thread_pool<T_event>::stop_workers() {
  // Assert that we have the workers_mutex_ and events_mutex_ locked
  assert(workers_mutex_.try_lock() == false &&
         "workers_mutex_ must be locked before calling stop_workers()");
  // Request stop for all workers
  for (auto& worker : workers_) {
    worker->request_stop();
  }
  // Wake up all workers so they can check stop_requested()
  cv_.notify_all();
  for (auto& worker : workers_) {
    if (worker->joinable()) {
      worker->join();
    }
  }
}

} // namespace thread_pool

} // namespace sequencer

} // namespace Micro_composer
