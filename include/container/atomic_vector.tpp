#include "container/atomic_vector.h"
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace Micro_composer {

namespace container {

// PUBLIC:

// Constructors
template <typename T>
Atomic_vector<T>::Atomic_vector(const Atomic_vector& other)
    : Base_vector(other) {
  store_size();
  // mutex_ is default-initialized
}

template <typename T>
Atomic_vector<T>& Atomic_vector<T>::operator=(const Atomic_vector& other) {
  std::scoped_lock lck{mutex_};
  if (this != &other) {
    Base_vector::operator=(other);
    store_size();
  }
  return *this;
}

template <typename T>
Atomic_vector<T>::Atomic_vector(Atomic_vector&& other) noexcept
    : Base_vector(std::move(other)) {
  // mutex_ is default-initialized
  store_size();
}

template <typename T>
template <typename Return_t, typename... Args>
const Return_t Atomic_vector<T>::under_lock(
    std::function<Return_t(std::vector<T>&, Args...)> func, Args... args) {
  // DEPRECATED
  std::scoped_lock lck{mutex_};
  return func(*this, args...);
}

template <typename T>
inline const std::scoped_lock<std::mutex>
Atomic_vector<T>::get_lock() const noexcept {
  return std::scoped_lock{mutex_};
}

template <typename T>
Atomic_vector<T>::Atomic_vector(const std::vector<T>& vec) : Base_vector(vec) {
  // mutex_ is default-initialized
}

template <typename T>
Atomic_vector<T>::Atomic_vector(std::vector<T>&& vec)
    : Base_vector(std::move(vec)) {
  // mutex_ is default-initialized
  store_size();
}

// CRUD Operations
template <typename T> void Atomic_vector<T>::assign(Initializer_list seq) {
  std::scoped_lock lck{mutex_};
  Base_vector::assign(seq);
  store_size();
}

template <typename T> void Atomic_vector<T>::assign(const std::vector<T>& vec) {
  std::scoped_lock lck{mutex_};
  Base_vector::assign(vec.begin(), vec.end());
  store_size();
}

template <typename T> void Atomic_vector<T>::push_back(const T& value) {
  std::scoped_lock lck{mutex_};
  Base_vector::push_back(value);
  store_size();
}

template <typename T> void Atomic_vector<T>::push_back(T&& value) {
  std::scoped_lock lck{mutex_};
  Base_vector::push_back(std::forward<T>(value));
  store_size();
}

template <typename T>
template <typename... Args>
void Atomic_vector<T>::emplace_back(Args&&... args) {
  std::scoped_lock lck{mutex_};
  Base_vector::emplace_back(std::forward<Args>(args)...);
  store_size();
}

template <typename T>
void Atomic_vector<T>::insert(Size_type pos, const T& value) {
  std::scoped_lock lck{mutex_};
  range_check(pos);
  Base_vector::insert(Base_vector::begin() + pos, value);
  store_size();
}

template <typename T> void Atomic_vector<T>::insert(Size_type pos, T&& value) {
  std::scoped_lock lck{mutex_};
  range_check(pos);
  Base_vector::insert(Base_vector::begin() + pos, std::forward<T>(value));
  store_size();
}

template <typename T>
void Atomic_vector<T>::assign(Size_type pos, const T& value) {
  std::scoped_lock lck{mutex_};
  range_check(pos);
  Base_vector::operator[](pos) = value;
}

template <typename T> void Atomic_vector<T>::assign(Size_type pos, T&& value) {
  std::scoped_lock lck{mutex_};
  range_check(pos);
  Base_vector::operator[](pos) = std::forward<T>(value);
}

template <typename T>
void Atomic_vector<T>::mutate(Size_type pos, const Mutator& fn) {
  std::scoped_lock lck{mutex_};
  Base_vector::operator[](pos) = fn(std::move(Base_vector::operator[](pos)));
}

template <typename T> void Atomic_vector<T>::mutate(const Mutator& fn) {
  std::scoped_lock lck{mutex_};
  for (auto& item : *this) {
    item = fn(std::move(item));
  }
}

template <typename T>
void Atomic_vector<T>::store(Size_type idx, const T& value) {
  std::scoped_lock lck{mutex_};
  Base_vector::operator[](idx) = value;
}

template <typename T> void Atomic_vector<T>::pop_back() {
  std::scoped_lock lck{mutex_};
  if (!Base_vector::empty()) {
    Base_vector::pop_back();
    store_size();
  }
}

template <typename T> void Atomic_vector<T>::erase(Size_type pos) {
  std::scoped_lock lck{mutex_};
  range_check(pos);
  Base_vector::erase(Base_vector::begin() + pos);
  store_size();
}

template <typename T> void Atomic_vector<T>::clear() noexcept {
  std::scoped_lock lck{mutex_};
  Base_vector::clear();
  store_size();
}

template <typename T> void Atomic_vector<T>::reserve(Size_type capacity) {
  std::scoped_lock lck{mutex_};
  Base_vector::reserve(capacity);
  store_size();
}

template <typename T>
inline typename Atomic_vector<T>::Size_type
Atomic_vector<T>::size() const noexcept {
  return size_.load(std::memory_order_acquire);
}

template <typename T>
inline typename Atomic_vector<T>::Size_type
Atomic_vector<T>::capacity() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_vector::capacity();
}

template <typename T> inline bool Atomic_vector<T>::empty() const noexcept {
  return size() == 0;
}

template <typename T> inline T Atomic_vector<T>::front() {
  std::scoped_lock lck{mutex_};
  return Base_vector::front();
}

template <typename T> inline T Atomic_vector<T>::back() {
  std::scoped_lock lck{mutex_};
  return Base_vector::back();
}

template <typename T> inline T Atomic_vector<T>::at(Size_type pos) {
  // Thread-safe at; returns a copy to prevent exposing a reference whose
  // value might be deleted by another thread.
  std::scoped_lock lck{mutex_};
  return Base_vector::at(pos);
}

template <typename T> inline T Atomic_vector<T>::operator[](Size_type pos) {
  std::scoped_lock lck{mutex_};
  return Base_vector::operator[](pos);
}

template <typename T>
inline typename Atomic_vector<T>::Base_vector
Atomic_vector<T>::snapshot() const noexcept {
  std::scoped_lock lck{mutex_};
  return Base_vector::data();
}

// Private

template <typename T> inline void Atomic_vector<T>::store_size() noexcept {
  size_.store(Base_vector::size(), std::memory_order_release);
}

template <typename T>
inline void Atomic_vector<T>::range_check(Size_type idx) const {
  if (idx >= size()) {
    throw std::out_of_range("[ERROR] In Atomic_vector: Index out of range.");
  }
}

} // namespace container
} // namespace Micro_composer