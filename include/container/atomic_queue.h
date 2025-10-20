#ifndef MICRO_COMPOSER_ATOMIC_QUEUE_H
#define MICRO_COMPOSER_ATOMIC_QUEUE_H

#include <atomic>
#include <mutex>
#include <queue>

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_queue : private std::queue<T> {

public:
  using Base_queue = std::queue<T>;
  using Size_type = typename Base_queue::size_type;

  // Constructors
  Atomic_queue() = default;
  Atomic_queue(const Atomic_queue& other);
  Atomic_queue& operator=(const Atomic_queue&);
  Atomic_queue(Atomic_queue&& other) noexcept;

  // Thread-safe CRUD operations
  void push(const T&);
  void push(T&&);

  void pop();

  T front();
  T back();

  Size_type size() const noexcept;
  bool empty() const noexcept;

private:
  mutable std::mutex mutex_;
  Base_queue queue_;
};

} // namespace container

} // namespace Micro_composer

#include "atomic_queue.tpp"

#endif
