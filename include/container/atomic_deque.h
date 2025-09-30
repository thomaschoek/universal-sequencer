#ifndef MICRO_COMPOSER_ATOMIC_DEQUE_H
#define MICRO_COMPOSER_ATOMIC_DEQUE_H

#include <deque>
#include <functional>
#include <initializer_list>
#include <mutex>

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_deque : protected std::deque<T> {
public:
  using Base_deque = std::deque<T>;
  using Index = Base_deque::size_type;
  using Initializer_list = std::initializer_list<T>;

  // Constructors
  Atomic_deque() = default;
  Atomic_deque(const Atomic_deque& other);
  Atomic_deque& operator=(const Atomic_deque&);
  Atomic_deque(Atomic_deque&& other) noexcept;

  Atomic_deque(Initializer_list seq);
  Atomic_deque(const std::vector<T>&);
  Atomic_deque(std::vector<T>&&);

  // Run anything under lock
  std::scoped_lock<std::mutex> get_lock();
  template <typename Return_type, typename... Args>
  const Return_type
  under_lock(std::function<Return_type(std::deque<T>, Args...)>, Args...);

  // Thread-safe CRUD operations
  void assign(Initializer_list);
  void assign(const std::vector<T>&);
  void assign(std::vector<T>&&);

  void push_back(const T&);
  void push_back(T&&);
  void push_front(const T&);
  void push_front(T&&);

  void insert(Index, const T&);
  void insert(Index, T&&);

  template <typename... Args> void update(Index, Args...);
  void replace(Index, const T&);
  void replace(Index, T&&);

  void pop_back();
  void pop_front();

  void erase(Index);

  void clear() noexcept;
  Index size() const noexcept;
  bool empty() const noexcept;

  T front();
  T back();

  T at(Index);

  const Base_deque& data() const noexcept;

private:
  mutable std::mutex mutex_;
};

} // namespace container

} // namespace Micro_composer

#include "container/atomic_deque.tpp"

#endif
