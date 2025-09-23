#ifndef MICRO_COMPOSER_EVENT_THREAD_POOL_H
#define MICRO_COMPOSER_EVENT_THREAD_POOL_H

#include <exception>
#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <functional>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace MicroComposer {

class EventThreadPool {
public:
  explicit EventThreadPool(size_t num_threads = std::thread::hardware_concurrency())
      : stop_flag_{false} {
    for (size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ~EventThreadPool() {
    stop_flag_ = true;
    condition_.notify_all();
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  template<typename F>
  void submit(F&& task) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      tasks_.emplace(std::forward<F>(task));
    }
    condition_.notify_one();
  }

private:
  void worker_loop() {
    while (!stop_flag_) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        condition_.wait(lock, [this] { return stop_flag_ || !tasks_.empty(); });

        if (stop_flag_ && tasks_.empty()) {
          break;
        }

        if (!tasks_.empty()) {
          task = std::move(tasks_.front());
          tasks_.pop();
        }
      }

      if (task) {
        try {
          task();
        } catch (const std::exception& e) {
          std::cerr << "[ERROR] Exception in event handler: " << e.what() << std::endl;
        }
      }
    }
  }

  std::vector<std::jthread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_flag_;
};

} // namespace MicroComposer

#endif // MICRO_COMPOSER_EVENT_THREAD_POOL_H