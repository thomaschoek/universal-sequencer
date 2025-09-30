#include "container/atomic_vector.h"
#include <mutex>
#include <stdexcept>

namespace Micro_composer {

namespace container {

// PUBLIC:

// Constructors
template <typename T>
Atomic_vector<T>::Atomic_vector(const Atomic_vector& other)
    : Base_vector(other) {
  // mutex_ is default-initialized
}

template <typename T>
Atomic_vector<T>& Atomic_vector<T>::operator=(const Atomic_vector& other) {
  if (this != &other) {
    std::scoped_lock lck{mutex_};
    Base_vector::operator=(other);
  }
  return *this;
}

template <typename T>
Atomic_vector<T>::Atomic_vector(Atomic_vector&& other) noexcept
    : Base_vector(std::move(other)) {
  // mutex_ is default-initialized
}

template <typename T>
template <typename Return_t, typename... Args>
const Return_t Atomic_vector<T>::under_lock(
    std::function<Return_t(std::vector<T>&, Args...)> func, Args... args) {
  std::scoped_lock lck{mutex_};
  return func(*this, args...);
}

template <typename T>
inline std::scoped_lock<std::mutex> Atomic_vector<T>::get_lock() {
  return std::scoped_lock{mutex_};
}

template <typename T>
Atomic_vector<T>::Atomic_vector(const std::vector<T>& vec)
    : Base_vector(vec) {
  // mutex_ is default-initialized
}

template <typename T>
Atomic_vector<T>::Atomic_vector(std::vector<T>&& vec)
    : Base_vector(std::move(vec)) {
  // mutex_ is default-initialized
}

// CRUD Operations

template <typename T> void Atomic_vector<T>::assign(Initializer_list seq) {
  std::scoped_lock lck{mutex_};
  Base_vector::assign(seq);
}

template <typename T> void Atomic_vector<T>::push_back(const T& value) {
  std::scoped_lock lck{mutex_};
  Base_vector::push_back(value);
}

template <typename T> void Atomic_vector<T>::push_back(T&& value) {
  std::scoped_lock lck{mutex_};
  Base_vector::push_back(std::forward<T>(value));
}

template <typename T>
template <typename... Args>
void Atomic_vector<T>::emplace_back(Args&&... args) {
  std::scoped_lock lck{mutex_};
  Base_vector::emplace_back(std::forward<Args>(args)...);
}

template <typename T> void Atomic_vector<T>::insert(Index pos, const T& value) {
  std::scoped_lock lck{mutex_};
  if (pos > Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::insert: Position out of range.");
  }
  Base_vector::insert(Base_vector::begin() + pos, value);
}

template <typename T> void Atomic_vector<T>::insert(Index pos, T&& value) {
  std::scoped_lock lck{mutex_};
  if (pos > Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::insert: Position out of range.");
  }
  Base_vector::insert(Base_vector::begin() + pos, std::forward<T>(value));
}

template <typename T>
void Atomic_vector<T>::replace(Index pos, const T& value) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::replace: Position out of range.");
  }
  Base_vector::at(pos) = value;
}

template <typename T> void Atomic_vector<T>::replace(Index pos, T&& value) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::replace: Position out of range.");
  }
  Base_vector::at(pos) = std::forward<T>(value);
}

template <typename T>
template <typename... Args>
void Atomic_vector<T>::update(Index pos, Args... args) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::update: Position out of range.");
  }
  Base_vector::at(pos).update(std::forward<Args>(args)...);
}

template <typename T> void Atomic_vector<T>::pop_back() {
  std::scoped_lock lck{mutex_};
  if (!Base_vector::empty()) {
    Base_vector::pop_back();
  }
}

template <typename T> void Atomic_vector<T>::erase(Index pos) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_vector::erase: Position out of range.");
  }
  Base_vector::erase(Base_vector::begin() + pos);
}

template <typename T> void Atomic_vector<T>::clear() noexcept {
  std::scoped_lock lck{mutex_};
  Base_vector::clear();
}

template <typename T> void Atomic_vector<T>::reserve(Index capacity) {
  std::scoped_lock lck{mutex_};
  Base_vector::reserve(capacity);
}

template <typename T>
inline typename Atomic_vector<T>::Index
Atomic_vector<T>::size() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_vector::size();
}

template <typename T>
inline typename Atomic_vector<T>::Index
Atomic_vector<T>::capacity() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_vector::capacity();
}

template <typename T> inline bool Atomic_vector<T>::empty() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_vector::empty();
}

template <typename T> inline T Atomic_vector<T>::front() {
  std::scoped_lock lck{mutex_};
  return Base_vector::front();
}

template <typename T> inline T Atomic_vector<T>::back() {
  std::scoped_lock lck{mutex_};
  return Base_vector::back();
}

template <typename T> inline T Atomic_vector<T>::at(Index pos) {
  // Thread-safe at; returns a copy to prevent exposing a reference whose
  // value might be deleted by another thread.
  std::scoped_lock lck{mutex_};
  return Base_vector::at(pos);
}

template <typename T> inline T& Atomic_vector<T>::operator[](Index pos) {
  std::scoped_lock lck{mutex_};
  return Base_vector::operator[](pos);
}

template <typename T>
inline const T& Atomic_vector<T>::operator[](Index pos) const {
  std::scoped_lock lck{mutex_};
  return Base_vector::operator[](pos);
}

template <typename T>
inline const typename Atomic_vector<T>::Base_vector&
Atomic_vector<T>::data() const noexcept {
  std::scoped_lock lck{mutex_};
  return *this;
}

} // namespace container
} // namespace Micro_composer