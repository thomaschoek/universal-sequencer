#include "atomic_queue.h"
#include <mutex>
#include <thread>

namespace Micro_composer {

namespace container {

// Constructors

template <typename T>
Atomic_queue<T>::Atomic_queue(const Atomic_queue& other) : Base_queue(other) {
  // mutex_ is default-initialized
}

template <typename T>
Atomic_queue<T>& Atomic_queue<T>::operator=(const Atomic_queue& other) {
  std::scoped_lock lck_consume{lock()};
  while (is_populating_.load(std::memory_order_acquire)) {
    ;
  }
  if (this != &other) {
    Base_queue::operator=(other);
  }
  return *this;
}

template <typename T>
Atomic_queue<T>::Atomic_queue(Atomic_queue&& other) noexcept
    : Base_queue(std::move(other)) {
  // mutex_ is default-initialized
}

// Thread-safe CRUD operations

template <typename T> void Atomic_queue<T>::push(const T& value) {
  is_populating_.store(true, std::memory_order_release);
  Base_queue::push(value);
  size_.store(Base_queue::size(), std::memory_order_release);
  is_populating_.store(false, std::memory_order_release);
}

template <typename T> void Atomic_queue<T>::push(T&& value) {
  is_populating_.store(true, std::memory_order_release);
  Base_queue::push(std::forward<T>(value));
  size_.store(Base_queue::size(), std::memory_order_release);
  is_populating_.store(false, std::memory_order_release);
}

template <typename T> void Atomic_queue<T>::pop() {
  std::scoped_lock lck{lock()};
  Base_queue::pop();
  size_.store(Base_queue::size(), std::memory_order_release);
}

template <typename T> void Atomic_queue<T>::pop(T& value) {
  std::scoped_lock lck{lock()};
  value = Base_queue::front();
  Base_queue::pop();
  size_.store(Base_queue::size(), std::memory_order_release);
}

template <typename T> T Atomic_queue<T>::pop_front() {
  std::scoped_lock lck{lock()};
  T value = Base_queue::front();
  Base_queue::pop();
  size_.store(Base_queue::size(), std::memory_order_release);
  return value;
}

template <typename T> T Atomic_queue<T>::front() {
  std::scoped_lock lck{lock()};
  return Base_queue::front();
}

template <typename T> T Atomic_queue<T>::back() {
  std::scoped_lock lck{lock()};
  return Base_queue::back();
}

template <typename T>
typename Atomic_queue<T>::Size_type Atomic_queue<T>::size() const noexcept {
  std::scoped_lock lck{lock()};
  return size_.load(std::memory_order_acquire);
}

template <typename T> bool Atomic_queue<T>::empty() const noexcept {
  return size() == 0;
}

// Private

template <typename T>
std::scoped_lock<std::mutex> Atomic_queue<T>::lock() const noexcept {
  while (is_populating_.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  return std::scoped_lock<std::mutex>(mutex_);
}

} // namespace container

} // namespace Micro_composer
