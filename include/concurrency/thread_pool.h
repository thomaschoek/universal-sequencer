#ifndef MICRO_COMPOSER_THREAD_POOL_H
#define MICRO_COMPOSER_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Micro_composer {

namespace sequencer {

namespace thread_pool {

template <typename T_event> struct Thread_pool {
  using Task = std::function<void(T_event&&)>;
  using Worker = std::jthread;
  using Worker_vector = std::vector<std::unique_ptr<Worker>>;
  using Event_deque = std::deque<std::unique_ptr<T_event>>;
  using Size_type = Worker_vector::size_type;

  Thread_pool(Task event_handler, Size_type initial_n_threads =
                                      std::thread::hardware_concurrency());

  // Prevent copying
  Thread_pool(const Thread_pool&) = delete;
  Thread_pool& operator=(const Thread_pool&) = delete;

  // Allow moving
  Thread_pool(Thread_pool&&) noexcept;
  Thread_pool& operator=(Thread_pool&&) noexcept;

  void submit(T_event&&);

private:
  void push_event(T_event&&);
  T_event&& pop_event();
  std::jthread worker();
  Task handler_;

  std::condition_variable cv_;
  std::mutex cv_mutex_;

  std::atomic<Size_type> workers_idle_{0};

  Worker_vector workers_;
  std::mutex workers_mutex_;

  Event_deque events_;
  std::atomic<Size_type> new_events_;
  std::mutex events_mutex_;
};

} // namespace thread_pool

} // namespace sequencer

} // namespace Micro_composer

#include "thread_pool.tpp"

#endif
