#include "atomic_deque.h"
#include <stdexcept>

namespace Micro_composer {

namespace atomic_deque {

template <typename T>
Atomic_deque<T>::Atomic_deque(size_type count) : base_type(count) {}

template <typename T>
Atomic_deque<T>::Atomic_deque(size_type count, const T& value)
    : base_type(count, value) {}

template <typename T> Atomic_deque<T>::Atomic_deque(const Atomic_deque& other) {
  std::scoped_lock<std::mutex> lock(other.mutex_);
  static_cast<base_type&>(*this) = static_cast<const base_type&>(other);
}

template <typename T>
Atomic_deque<T>::Atomic_deque(Atomic_deque&& other) noexcept {
  std::scoped_lock<std::mutex> lock(other.mutex_);
  static_cast<base_type&>(*this) = std::move(static_cast<base_type&>(other));
}

template <typename T>
Atomic_deque<T>& Atomic_deque<T>::operator=(const Atomic_deque& other) {
  if (this != &other) {
    std::scoped_lock lck(mutex_, other.mutex_);
    static_cast<base_type&>(*this) = static_cast<const base_type&>(other);
  }
  return *this;
}

template <typename T>
Atomic_deque<T>& Atomic_deque<T>::operator=(Atomic_deque&& other) noexcept {
  if (this != &other) {
    std::scoped_lock lck(mutex_, other.mutex_);
    static_cast<base_type&>(*this) = std::move(static_cast<base_type&>(other));
  }
  return *this;
}

template <typename T> void Atomic_deque<T>::push_back(const T& value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_back(value);
}

template <typename T> void Atomic_deque<T>::push_back(T&& value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_back(std::move(value));
}

template <typename T> void Atomic_deque<T>::push_front(const T& value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_front(value);
}

template <typename T> void Atomic_deque<T>::push_front(T&& value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_front(std::move(value));
}

template <typename T> void Atomic_deque<T>::pop_back() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (!base_type::empty()) {
    base_type::pop_back();
  }
}

template <typename T> void Atomic_deque<T>::pop_front() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (!base_type::empty()) {
    base_type::pop_front();
  }
}

template <typename T> void Atomic_deque<T>::clear() {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::clear();
}

template <typename T>
typename Atomic_deque<T>::size_type Atomic_deque<T>::size() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::size();
}

template <typename T> bool Atomic_deque<T>::empty() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::empty();
}

template <typename T> T& Atomic_deque<T>::front() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::front() called on empty deque");
  }
  return base_type::front();
}

template <typename T> const T& Atomic_deque<T>::front() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::front() called on empty deque");
  }
  return base_type::front();
}

template <typename T> T& Atomic_deque<T>::back() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::back() called on empty deque");
  }
  return base_type::back();
}

template <typename T> const T& Atomic_deque<T>::back() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::back() called on empty deque");
  }
  return base_type::back();
}

template <typename T> T& Atomic_deque<T>::at(size_type pos) {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::at(pos);
}

template <typename T> const T& Atomic_deque<T>::at(size_type pos) const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::at(pos);
}

template <typename T> T& Atomic_deque<T>::operator[](size_type pos) {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::operator[](pos);
}

template <typename T>
const T& Atomic_deque<T>::operator[](size_type pos) const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::operator[](pos);
}

template <typename T>
inline std::scoped_lock<std::mutex> Atomic_deque<T>::lock() const {
  return std::scoped_lock<std::mutex>(mutex_);
}

} // namespace atomic_deque

} // namespace Micro_composer