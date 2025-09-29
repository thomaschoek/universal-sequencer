#ifndef MICRO_COMPOSER_RING_DEQUE_H
#define MICRO_COMPOSER_RING_DEQUE_H

#include <deque>

namespace Micro_composer {

namespace container {

template <typename Step_t> class Ring_deque : public std::deque<Step_t> {
public:
  using Base_deque = std::deque<Step_t>;
  Step_t next();

protected:
  const std::deque<Step_t>::iterator iterator_;
};

} // namespace container

} // namespace Micro_composer

#endif
