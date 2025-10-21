#ifndef MICRO_COMPOSER_RING_VECTOR_H
#define MICRO_COMPOSER_RING_VECTOR_H

#include <vector>

namespace Micro_composer {

namespace container {

template <typename T> class Ring_vector : public std::vector<T> {
public:
  using Base_vector = std::vector<T>;
  using Const_iterator = Base_vector::const_iterator;
  using Size_type = Base_vector::size_type;
  using Initializer_list = std::initializer_list<Base_vector>;
  // Constructors
  Ring_vector() = default;
  Ring_vector(const Ring_vector&) = default;
  Ring_vector& operator=(const Ring_vector&) = default;
  Ring_vector(Ring_vector&&) noexcept = default;
  Ring_vector& operator=(Ring_vector&&) noexcept = default;

  Ring_vector(Initializer_list seq);
  Ring_vector(const std::vector<T>& vec);
  Ring_vector(std::vector<T>&& vec);

  Const_iterator next();
  void set_next(Size_type = 0);

private:
  Const_iterator iterator_{Base_vector::cbegin()};
};

} // namespace container

} // namespace Micro_composer

#include "ring_vector.tpp"

#endif
