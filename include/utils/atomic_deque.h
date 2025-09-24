#ifndef MICRO_COMPOSER_ATOMIC_DEQUE_H
#define MICRO_COMPOSER_ATOMIC_DEQUE_H

#include <deque>
#include <mutex>

namespace Micro_composer {

namespace atomic_deque {

template <typename T> class Atomic_deque : public std::deque<T> {
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
  Atomic_deque() = default;
  explicit Atomic_deque(size_type count);
  Atomic_deque(size_type count, const T& value);
  Atomic_deque(const Atomic_deque& other);
  Atomic_deque(Atomic_deque&& other) noexcept;

  // Assignment operators
  Atomic_deque& operator=(const Atomic_deque& other);
  Atomic_deque& operator=(Atomic_deque&& other) noexcept;

  // Destructor
  ~Atomic_deque() = default;

  // Thread-safe operations
  void push_back(const T& value);
  void push_back(T&& value);
  void push_front(const T& value);
  void push_front(T&& value);

  void pop_back();
  void pop_front();

  void clear();
  size_type size() const;
  bool empty() const;

  T& front();
  const T& front() const;
  T& back();
  const T& back() const;

  T& at(size_type pos);
  const T& at(size_type pos) const;
  T& operator[](size_type pos);
  const T& operator[](size_type pos) const;

  std::scoped_lock<std::mutex> lock() const;
};

} // namespace atomic_deque

} // namespace Micro_composer

#endif // ATOMIC_DEQUE_H