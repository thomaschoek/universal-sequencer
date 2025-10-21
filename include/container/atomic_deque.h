#ifndef MICRO_COMPOSER_ATOMIC_DEQUE_H
#define MICRO_COMPOSER_ATOMIC_DEQUE_H

#include <atomic>
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
  std::scoped_lock<std::mutex> get_lock() const;
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
  T at(Index) const;

  const Base_deque& data() const noexcept;

private:
  void update_dimensions() noexcept;
  mutable std::mutex mutex_;

  std::atomic<Index> size_{0};
  std::atomic<typename Base_deque::iterator> begin_{Base_deque::begin()};
  std::atomic<typename Base_deque::iterator> end_{Base_deque::end()};
  std::atomic<typename Base_deque::const_iterator> cbegin_{
      Base_deque::cbegin()};
  std::atomic<typename Base_deque::const_iterator> cend_{Base_deque::cend()};
};

} // namespace container

} // namespace Micro_composer

#include "container/atomic_deque.tpp"

#endif
