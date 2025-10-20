#include "container/atomic_deque.h"
#include <mutex>
#include <stdexcept>

namespace Micro_composer {

namespace container {

// PUBLIC:

// Constructors
template <typename T>
Atomic_deque<T>::Atomic_deque(const Atomic_deque& other) : Base_deque(other) {
  // mutex_ is default-initialized
  update_dimensions();
}

template <typename T>
Atomic_deque<T>::Atomic_deque(Atomic_deque&& other) noexcept
    : Base_deque(std::move(other)) {
  // mutex_ is default-initialized
  update_dimensions();
}

template <typename T>
Atomic_deque<T>::Atomic_deque(Initializer_list seq) : Base_deque(seq) {
  // mutex_ is default-initialized
  update_dimensions();
}

template <typename T>
Atomic_deque<T>::Atomic_deque(const std::vector<T>& vec)
    : Base_deque(vec.begin(), vec.end()) {
  // mutex_ is default-initialized
  update_dimensions();
}

template <typename T>
Atomic_deque<T>::Atomic_deque(std::vector<T>&& vec)
    : Base_deque(std::make_move_iterator(vec.begin()),
                 std::make_move_iterator(vec.end())) {
  // mutex_ is default-initialized
  update_dimensions();
}

// Operators
template <typename T>
Atomic_deque<T>& Atomic_deque<T>::operator=(const Atomic_deque& other) {
  std::scoped_lock lck{mutex_};
  if (this != &other) {
    Base_deque::operator=(other);
    update_dimensions();
  }
  return *this;
}

// Utility
template <typename T>
template <typename Return_t, typename... Args>
const Return_t Atomic_deque<T>::under_lock(
    std::function<Return_t(std::deque<T>, Args...)> func, Args... args) {
  // DEPRECATED
  std::scoped_lock lck{mutex_};
  return func(&this, args...);
}

template <typename T>
inline std::scoped_lock<std::mutex> Atomic_deque<T>::get_lock() {
  return std::scoped_lock{mutex_};
}

template <typename T>
inline std::scoped_lock<std::mutex> Atomic_deque<T>::get_lock() const {
  return std::scoped_lock{mutex_};
}

// CRUD Operations

template <typename T> void Atomic_deque<T>::assign(Initializer_list seq) {
  std::scoped_lock lck{mutex_};
  Base_deque::assign(seq);
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::assign(const std::vector<T>& vec) {
  std::scoped_lock lck{mutex_};
  Base_deque::assign(vec.begin(), vec.end());
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::assign(std::vector<T>&& vec) {
  std::scoped_lock lck{mutex_};
  Base_deque::assign(std::make_move_iterator(vec.begin()),
                     std::make_move_iterator(vec.end()));
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::push_back(const T& value) {
  std::scoped_lock lck{mutex_};
  Base_deque::push_back(value);
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::push_back(T&& value) {
  std::scoped_lock lck{mutex_};
  Base_deque::push_back(std::forward<T>(value));
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::push_front(const T& value) {
  std::scoped_lock lck{mutex_};
  Base_deque::push_front(value);
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::push_front(T&& value) {
  std::scoped_lock lck{mutex_};
  Base_deque::push_front(std::forward<T>(value));
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::insert(Index pos, const T& value) {
  std::scoped_lock lck{mutex_};
  if (pos > Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::insert: Position out of range.");
  }
  Base_deque::insert(Base_deque::begin() + pos, value);
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::insert(Index pos, T&& value) {
  std::scoped_lock lck{mutex_};
  if (pos > Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::insert: Position out of range.");
  }
  Base_deque::insert(Base_deque::begin() + pos, std::forward<T>(value));
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::replace(Index pos, const T& value) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::replace: Position out of range.");
  }
  Base_deque::at(pos) = value;
}

template <typename T> void Atomic_deque<T>::replace(Index pos, T&& value) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::replace: Position out of range.");
  }
  Base_deque::at(pos) = std::forward<T>(value);
}

template <typename T>
template <typename... Args>
void Atomic_deque<T>::update(Index pos, Args... args) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_deque::update: Position out of range.");
  }
  Base_deque::at(pos).update(std::forward<Args>(args)...);
}

template <typename T> void Atomic_deque<T>::pop_back() {
  std::scoped_lock lck{mutex_};
  Base_deque::pop_back();
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::pop_front() {
  std::scoped_lock lck{mutex_};
  Base_deque::pop_front();
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::erase(Index pos) {
  std::scoped_lock lck{mutex_};
  if (pos >= Base_deque::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_deque::erase: Position out of range.");
  }
  Base_deque::erase(Base_deque::begin() + pos);
  update_dimensions();
}

template <typename T> void Atomic_deque<T>::clear() noexcept {
  std::scoped_lock lck{mutex_};
  Base_deque::clear();
  update_dimensions();
}

template <typename T>
inline Atomic_deque<T>::Index Atomic_deque<T>::size() const noexcept {
  return size_.load(std::memory_order_acquire);
}

template <typename T> inline bool Atomic_deque<T>::empty() const noexcept {
  return size_.load(std::memory_order_acquire) == 0;
}

template <typename T> inline T Atomic_deque<T>::front() {
  std::scoped_lock lck{mutex_};
  return Base_deque::front();
}

template <typename T> inline T Atomic_deque<T>::back() {
  std::scoped_lock lck{mutex_};
  return Base_deque::back();
}

template <typename T> inline T Atomic_deque<T>::at(Index pos) {
  // Thread-safe at; rather than T&, return a copy of Base_deque::at(pos)
  // to prevent exposing a reference whose value might be deleted by another
  // thread.
  std::scoped_lock lck{mutex_};
  return Base_deque::at(pos);
}

template <typename T> inline T Atomic_deque<T>::at(Index pos) const {
  // Const version for read-only access
  std::scoped_lock lck{mutex_};
  return Base_deque::at(pos);
}

template <typename T>
inline const Atomic_deque<T>::Base_deque&
Atomic_deque<T>::data() const noexcept {
  std::scoped_lock lck{mutex_};
  return *this;
}

// Private

template <typename T> void Atomic_deque<T>::update_dimensions() {
  begin_.store(Base_deque::begin(), std::memory_order_release);
  cbegin_.store(Base_deque::cbegin(), std::memory_order_release);
  end_.store(Base_deque::end(), std::memory_order_release);
  cend_.store(Base_deque::cend(), std::memory_order_release);
  size_.store(Base_deque::size(), std::memory_order_release);
}

} // namespace container
} // namespace Micro_composer
