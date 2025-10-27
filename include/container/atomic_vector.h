#ifndef MICRO_COMPOSER_ATOMIC_VECTOR_H
#define MICRO_COMPOSER_ATOMIC_VECTOR_H

#include <atomic>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <vector>

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_vector : private std::vector<T> {
public:
  using Base_vector = std::vector<T>;
  using Size_type = Base_vector::size_type;
  using Initializer_list = std::initializer_list<T>;
  using Mutator = std::function<T(T&&)>;

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
  void assign(Size_type, const T&);
  void assign(Initializer_list);
  void assign(const std::vector<T>&);

  void push_back(const T&);
  void push_back(T&&);
  template <typename... Args> void emplace_back(Args&&...);

  void insert(Size_type, const T&);
  void insert(Size_type, T&&);

  // Atomic updates on individual elements
  void mutate(Size_type, const Mutator&);
  void mutate(const Mutator&);
  void store(Size_type, const T&);

  void pop_back();
  void erase(Size_type);

  void clear() noexcept;
  void reserve(Size_type);

  Size_type size() const noexcept;
  Size_type capacity() const noexcept;
  bool empty() const noexcept;

  T front();
  T back();
  T at(Size_type);

  T operator[](Size_type);

  Base_vector snapshot() const noexcept;

  void range_check(Size_type) const;

private:
  void store_size() noexcept;
  mutable std::mutex mutex_;
  mutable std::atomic<Size_type> size_{0};
};

} // namespace container

} // namespace Micro_composer

#include "container/atomic_vector.tpp"

#endif