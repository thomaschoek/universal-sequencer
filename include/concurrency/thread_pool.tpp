#include "thread_pool.h"
#ifndef NDEBUG
#include <iostream>
#include <syncstream>
#endif
#include <cassert>

namespace Micro_composer {

namespace sequencer {

namespace thread_pool {

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
#ifndef NDEBUG
  {
    std::osyncstream(std::cerr)
        << "[THREAD_POOL] Destructor starting, workers_.size()="
        << workers_.size() << "\n"
        << std::flush;
  }
#endif
  std::scoped_lock lck{workers_mutex_, events_mutex_};
  stop_workers();
#ifndef NDEBUG
  {
    std::osyncstream(std::cerr) << "[THREAD_POOL] Notified all workers\n"
                                << std::flush;
  }
#endif
  // jthread destructors will request stop and join automatically
#ifndef NDEBUG
  {
    std::osyncstream(std::cerr) << "[THREAD_POOL] Destructor exiting\n"
                                << std::flush;
  }
#endif
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
  const Size_type workers_idle = workers_idle_.load(std::memory_order_acquire);
  if (workers_idle > 1 && workers_idle > std::thread::hardware_concurrency()) {
    // Too many idle workers, remove one
    workers_.pop_back();
  }
}

// Private
template <sequencable::Sequencable T_event>
void Thread_pool<T_event>::push_event(T_event&& evt) {
  {
    std::scoped_lock lck{events_mutex_};
    events_.emplace_back(std::make_unique<T_event>(std::move(evt)));
    new_events_.fetch_add(1, std::memory_order_acq_rel);
  }
}

template <sequencable::Sequencable T_event>
inline T_event Thread_pool<T_event>::pop_event() {
  T_event event;
  {
    std::scoped_lock lck{events_mutex_};
    event = std::move(*events_.front());
    events_.pop_front();
    new_events_.fetch_sub(1, std::memory_order_acq_rel);
  }
  return event;
}

template <sequencable::Sequencable T_event>
std::jthread Thread_pool<T_event>::worker() {
  return std::jthread([this](std::stop_token st) {
#ifndef NDEBUG
    {
      std::osyncstream(std::cerr) << "[THREAD_POOL] Worker "
                                  << std::this_thread::get_id() << " started\n"
                                  << std::flush;
    }
#endif
    while (!st.stop_requested()) {
      workers_idle_.fetch_add(1, std::memory_order_acq_rel);
      {
#ifndef NDEBUG
        {
          std::osyncstream(std::cerr)
              << "[THREAD_POOL] Worker " << std::this_thread::get_id()
              << " acquiring cv_mutex_...\n"
              << std::flush;
        }
#endif
        std::unique_lock lck{cv_mutex_};
        while (!st.stop_requested() &&
               new_events_.load(std::memory_order_acquire) == 0) {
#ifndef NDEBUG
          {
            std::osyncstream(std::cerr)
                << "[THREAD_POOL] Worker " << std::this_thread::get_id()
                << " new_events_.load() returned 0\n"
                << std::flush;
          }
#endif
#ifndef NDEBUG
          {
            std::osyncstream(std::cerr)
                << "[THREAD_POOL] Worker " << std::this_thread::get_id()
                << " WAITING...\n"
                << std::flush;
          }
#endif
          cv_.wait(lck);
#ifndef NDEBUG
          {
            std::osyncstream(std::cerr)
                << "[THREAD_POOL] Worker " << std::this_thread::get_id()
                << " NOTIFIED\n"
                << std::flush;
          }
#endif
        }
        workers_idle_.fetch_sub(1, std::memory_order_acq_rel);
        if (st.stop_requested()) {
          return;
        }
      }
      T_event event = pop_event();
      const Time_point& scheduled_time = event.scheduled_time;
#ifndef NDEBUG
      {
        std::osyncstream(std::cerr)
            << "[THREAD_POOL] Worker " << std::this_thread::get_id()
            << " POPPED EVENT with scheduled_time "
            << duration_cast<std::chrono::milliseconds>(
                   scheduled_time.time_since_epoch())
                   .count()
            << "\n"
            << std::flush;
      }
#endif
      std::this_thread::sleep_until(event.scheduled_time - spin_duration_);
      while (Clock::now() < event.scheduled_time) {
        if (st.stop_requested()) {
          return;
        }
      }
#ifndef NDEBUG
      {
        std::osyncstream(std::cerr)
            << "[THREAD_POOL] Worker " << std::this_thread::get_id()
            << " CALLING HANDLER\n"
            << std::flush;
      }
#endif
      handler_(std::forward<T_event>(event));
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
