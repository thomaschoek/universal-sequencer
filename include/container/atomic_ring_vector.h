#ifndef MICRO_COMPOSER_ATOMIC_RING_VECTOR_H
#define MICRO_COMPOSER_ATOMIC_RING_VECTOR_H

#include "container/atomic_vector.h"

namespace Micro_composer {

namespace container {

template <typename T> class Atomic_ring_vector : public Atomic_vector<T> {
public:
  using Base_vector = Atomic_vector<T>;

  // Constructors
  Atomic_ring_vector() = default;
  Atomic_ring_vector(const Atomic_ring_vector&) = default;
  Atomic_ring_vector& operator=(const Atomic_ring_vector&) = default;
  Atomic_ring_vector(Atomic_ring_vector&&) noexcept = default;
  Atomic_ring_vector& operator=(Atomic_ring_vector&&) noexcept = default;

  Atomic_ring_vector(typename Base_vector::Initializer_list seq);
  Atomic_ring_vector(const std::vector<T>& vec);
  Atomic_ring_vector(std::vector<T>&& vec);

  T next();
  void set_pos(typename Base_vector::Index = 0);
  typename Base_vector::Index get_pos() const;

  // Override CRUD operations to handle iterator invalidation
  void assign(typename Base_vector::Initializer_list);
  void assign(const std::vector<T>&);
  void push_back(const T&);
  void insert(typename Base_vector::Index, const T&);
  void erase(typename Base_vector::Index);
  void clear() noexcept;

private:
  // Workaround: only for use under lock to avoid double locking
  using Unatomic_base_vector = Base_vector::Base_vector;
  Base_vector::const_iterator iterator_{Base_vector::begin()};
};

} // namespace container

} // namespace Micro_composer

#include "atomic_ring_vector.tpp"

#endif
