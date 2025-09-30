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

  // Constructors
  Atomic_vector() = default;
  Atomic_vector(const Atomic_vector& other);
  Atomic_vector& operator=(const Atomic_vector&);
  Atomic_vector(Atomic_vector&& other) noexcept;

  Atomic_vector(std::vector<T>&&) noexcept;

  // Run anything under lock
  std::scoped_lock<std::mutex> get_lock();
  template <typename Return_type, typename... Args>
  const Return_type
  under_lock(std::function<Return_type(std::vector<T>&, Args...)>, Args...);

  // Thread-safe CRUD operations
  void assign(Initializer_list);

  void push_back(const T&);
  void push_back(T&&);
  template <typename... Args> void emplace_back(Args&&...);

  void insert(Index, const T&);
  void insert(Index, T&&);

  void replace(Index, const T&);
  void replace(Index, T&&);

  void pop_back();
  void erase(Index);

  void clear() noexcept;
  void reserve(Index);

  Index size() const noexcept;
  Index capacity() const noexcept;
  bool empty() const noexcept;

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