#ifndef MICRO_COMPOSER_ATOMIC_VECTOR_H
#define MICRO_COMPOSER_ATOMIC_VECTOR_H

#include <functional>
#include <initializer_list>
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_vector : protected std::vector<T> {
public:
  using Base_vector = std::vector<T>;
  using Index = typename Base_vector::size_type;
  using Initializer_list = std::initializer_list<T>;
  using Iterator = Base_vector::iterator;
  using Const_iterator = Base_vector::const_iterator;

  // Constructors
  Atomic_vector() = default;
  Atomic_vector(const Atomic_vector& other);
  Atomic_vector& operator=(const Atomic_vector&);
  Atomic_vector(Atomic_vector&& other) noexcept;

  Atomic_vector(const std::vector<T>&);
  Atomic_vector(std::vector<T>&&);

  // Run anything under lock
  const std::scoped_lock<std::mutex> get_lock() const noexcept;
  template <typename Return_type, typename... Args>
  const Return_type
  under_lock(std::function<Return_type(std::vector<T>&, Args...)>, Args...);

  // Thread-safe CRUD operations
  void assign(Index, const T&);
  void assign(Index, T&&);
  void assign(Initializer_list);
  void assign(const std::vector<T>&);

  void push_back(const T&);
  void push_back(T&&);
  template <typename... Args> void emplace_back(Args&&...);

  void insert(Index, const T&);
  void insert(Index, T&&);

  template <typename... Args> void update(Index, Args...);

  void pop_back();
  void erase(Index);

  void clear() noexcept;
  void reserve(Index);

  Index size() const noexcept;
  Index capacity() const noexcept;
  bool empty() const noexcept;

  Const_iterator cbegin() const noexcept;
  Const_iterator cend() const noexcept;
  T front();
  T back();
  T at(Index);

  // WARNING: References returned are only safe while no other thread
  // modifies the vector structure (add/remove elements)
  T& operator[](Index);
  const T& operator[](Index) const;

  const Base_vector& data() const noexcept;

private:
  mutable std::mutex mutex_;
};

} // namespace container

} // namespace Micro_composer

#include "container/atomic_vector.tpp"

#endif