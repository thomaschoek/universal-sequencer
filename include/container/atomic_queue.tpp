#include "atomic_queue.h"
#include <mutex>

namespace Micro_composer {

namespace container {

// Constructors

template <typename T>
Atomic_queue<T>::Atomic_queue(const Atomic_queue& other) : Base_queue(other) {
  // mutex_ is default-initialized
}

template <typename T>
Atomic_queue<T>& Atomic_queue<T>::operator=(const Atomic_queue& other) {
  if (this != &other) {
    std::scoped_lock lck{mutex_};
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
  std::scoped_lock lck{mutex_};
  Base_queue::push(value);
}

template <typename T> void Atomic_queue<T>::push(T&& value) {
  std::scoped_lock lck{mutex_};
  Base_queue::push(std::forward<T>(value));
}

template <typename T> void Atomic_queue<T>::pop() {
  std::scoped_lock lck{mutex_};
  if (!Base_queue::empty()) {
    Base_queue::pop();
  }
}

template <typename T> T Atomic_queue<T>::front() {
  std::scoped_lock lck{mutex_};
  return Base_queue::front();
}

template <typename T> T Atomic_queue<T>::back() {
  std::scoped_lock lck{mutex_};
  return Base_queue::back();
}

template <typename T>
typename Atomic_queue<T>::Size_type Atomic_queue<T>::size() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_queue::size();
}

template <typename T> bool Atomic_queue<T>::empty() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_queue::empty();
}

} // namespace container

} // namespace Micro_composer
