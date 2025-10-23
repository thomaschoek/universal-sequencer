#include "thread_pool.h"

namespace Micro_composer {

namespace sequencer {

namespace thread_pool {

// Constructors

template <typename T_event>
Thread_pool<T_event>::Thread_pool(Task event_handler,
                                  Size_type initial_n_threads)
    : handler_(event_handler) {
  std::scoped_lock lck{workers_mutex_};
  for (Size_type i = 0; i < initial_n_threads; ++i) {
    workers_.push_back(std::make_unique<std::jthread>(worker()));
  }
}

template <typename T_event>
Thread_pool<T_event>::Thread_pool(Thread_pool&& other) noexcept
    : handler_(std::move(other.handler_)) {
  {
    std::scoped_lock lck{other.workers_mutex_};
    workers_ = std::move(other.workers_);
  }
  {
    std::scoped_lock lck{other.events_mutex_};
    events_ = std::move(other.events_);
  }
}

template <typename T_event>
Thread_pool<T_event>&
Thread_pool<T_event>::operator=(Thread_pool&& other) noexcept {
  if (this != &other) {
    handler_ = std::move(other.handler_);
    {
      std::scoped_lock lck1{workers_mutex_, other.workers_mutex_};
      workers_ = std::move(other.workers_);
    }
    {
      std::scoped_lock lck2{events_mutex_, other.events_mutex_};
      events_ = std::move(other.events_);
    }
  }
  return *this;
}

// Public

template <typename T_event>
void Thread_pool<T_event>::set_handler(const Task& t) {
  std::scoped_lock lock_cv{cv_mutex_};
  std::scoped_lock lock_workers{workers_mutex_};
  while (workers_idle_.load(std::memory_order_acquire) < workers_.size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  handler_ = t;
}

template <typename T_event> void Thread_pool<T_event>::submit(T_event&& event) {
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
template <typename T_event>
void Thread_pool<T_event>::push_event(T_event&& evt) {
  {
    std::scoped_lock lck{events_mutex_};
    events_.emplace_back(std::make_unique<T_event>(std::move(evt)));
    new_events_.fetch_add(1, std::memory_order_acq_rel);
  }
}

template <typename T_event> inline T_event&& Thread_pool<T_event>::pop_event() {
  std::unique_ptr<T_event> event_ptr;
  {
    std::scoped_lock lck{events_mutex_};
    event_ptr = std::move(events_.front());
    events_.pop_front();
    new_events_.fetch_sub(1, std::memory_order_acq_rel);
  }
  return std::move(*event_ptr);
}

template <typename T_event> std::jthread Thread_pool<T_event>::worker() {
  return std::jthread([this](std::stop_token st) {
    while (!st.stop_requested()) {
      workers_idle_.fetch_add(1, std::memory_order_acq_rel);
      {
        std::unique_lock lck{cv_mutex_};
        while (new_events_.load(std::memory_order_acquire) == 0 &&
               !st.stop_requested()) {
          cv_.wait(lck);
        }
        workers_idle_.fetch_sub(1, std::memory_order_acq_rel);
      }
      handler_(pop_event());
    }
  });
}

} // namespace thread_pool

} // namespace sequencer

} // namespace Micro_composer
