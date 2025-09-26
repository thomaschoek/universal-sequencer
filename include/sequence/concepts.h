#ifndef MICRO_COMPOSER_SEQUENCE_CONCEPTS_H
#define MICRO_COMPOSER_SEQUENCE_CONCEPTS_H

namespace Micro_composer {

namespace sequence {

template <typename T>
concept Sequence = requires(T sequence) {
  T::iterator;
  T::size_type;
  T::Event_t;
};

} // namespace sequence

} // namespace Micro_composer

#endif
