#ifndef ATOMIC_DEQUE_H
#define ATOMIC_DEQUE_H

#include <deque>
#include <mutex>
#include <stdexcept>

namespace atomic_deque {

template <typename T> class AtomicDeque : public std::deque<T> {
public:
  using base_type = std::deque<T>;
  using value_type = typename base_type::value_type;
  using size_type = typename base_type::size_type;
  using iterator = typename base_type::iterator;
  using const_iterator = typename base_type::const_iterator;

protected:
  mutable std::mutex mutex_;

public:
  // Constructors
  AtomicDeque() = default;
  explicit AtomicDeque(size_type count);
  AtomicDeque(size_type count, const T &value);
  AtomicDeque(const AtomicDeque &other);
  AtomicDeque(AtomicDeque &&other) noexcept;

  // Assignment operators
  AtomicDeque &operator=(const AtomicDeque &other);
  AtomicDeque &operator=(AtomicDeque &&other) noexcept;

  // Destructor
  ~AtomicDeque() = default;

  // Thread-safe operations
  void push_back(const T &value);
  void push_back(T &&value);
  void push_front(const T &value);
  void push_front(T &&value);

  void pop_back();
  void pop_front();

  void clear();
  size_type size() const;
  bool empty() const;

  T &front();
  const T &front() const;
  T &back();
  const T &back() const;

  T &at(size_type pos);
  const T &at(size_type pos) const;
  T &operator[](size_type pos);
  const T &operator[](size_type pos) const;
};

template <typename T>
AtomicDeque<T>::AtomicDeque(size_type count) : base_type(count) {}

template <typename T>
AtomicDeque<T>::AtomicDeque(size_type count, const T &value)
    : base_type(count, value) {}

template <typename T> AtomicDeque<T>::AtomicDeque(const AtomicDeque &other) {
  std::scoped_lock<std::mutex> lock(other.mutex_);
  static_cast<base_type &>(*this) = static_cast<const base_type &>(other);
}

template <typename T>
AtomicDeque<T>::AtomicDeque(AtomicDeque &&other) noexcept {
  std::scoped_lock<std::mutex> lock(other.mutex_);
  static_cast<base_type &>(*this) = std::move(static_cast<base_type &>(other));
}

template <typename T>
AtomicDeque<T> &AtomicDeque<T>::operator=(const AtomicDeque &other) {
  if (this != &other) {
    std::scoped_lock lck(mutex_, other.mutex_);
    static_cast<base_type &>(*this) = static_cast<const base_type &>(other);
  }
  return *this;
}

template <typename T>
AtomicDeque<T> &AtomicDeque<T>::operator=(AtomicDeque &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lck(mutex_, other.mutex_);
    static_cast<base_type &>(*this) =
        std::move(static_cast<base_type &>(other));
  }
  return *this;
}

template <typename T> void AtomicDeque<T>::push_back(const T &value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_back(value);
}

template <typename T> void AtomicDeque<T>::push_back(T &&value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_back(std::move(value));
}

template <typename T> void AtomicDeque<T>::push_front(const T &value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_front(value);
}

template <typename T> void AtomicDeque<T>::push_front(T &&value) {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::push_front(std::move(value));
}

template <typename T> void AtomicDeque<T>::pop_back() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (!base_type::empty()) {
    base_type::pop_back();
  }
}

template <typename T> void AtomicDeque<T>::pop_front() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (!base_type::empty()) {
    base_type::pop_front();
  }
}

template <typename T> void AtomicDeque<T>::clear() {
  std::scoped_lock<std::mutex> lock(mutex_);
  base_type::clear();
}

template <typename T>
typename AtomicDeque<T>::size_type AtomicDeque<T>::size() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::size();
}

template <typename T> bool AtomicDeque<T>::empty() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::empty();
}

template <typename T> T &AtomicDeque<T>::front() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::front() called on empty deque");
  }
  return base_type::front();
}

template <typename T> const T &AtomicDeque<T>::front() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::front() called on empty deque");
  }
  return base_type::front();
}

template <typename T> T &AtomicDeque<T>::back() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::back() called on empty deque");
  }
  return base_type::back();
}

template <typename T> const T &AtomicDeque<T>::back() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (base_type::empty()) {
    throw std::out_of_range("atomic_deque::back() called on empty deque");
  }
  return base_type::back();
}

template <typename T> T &AtomicDeque<T>::at(size_type pos) {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::at(pos);
}

template <typename T> const T &AtomicDeque<T>::at(size_type pos) const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::at(pos);
}

template <typename T> T &AtomicDeque<T>::operator[](size_type pos) {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::operator[](pos);
}

template <typename T> const T &AtomicDeque<T>::operator[](size_type pos) const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return base_type::operator[](pos);
}

} // namespace atomic_deque

#endif // ATOMIC_DEQUE_H