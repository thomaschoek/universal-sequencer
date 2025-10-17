#include "container/atomic_ring_vector.h"
#include <stdexcept>

namespace Micro_composer {

namespace container {

// Constructors
template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(
    typename Base_vector::Initializer_list seq)
    : Base_vector(seq) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(const std::vector<T>& vec)
    : Base_vector(vec) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
Atomic_ring_vector<T>::Atomic_ring_vector(std::vector<T>&& vec)
    : Base_vector(std::move(vec)) {
  // mutex_ is default-initialized
  iterator_ = Base_vector::cbegin();
}

template <typename T>
void Atomic_ring_vector<T>::set_next(typename Base_vector::Index pos) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (Unatomic_base_vector::empty()) {
    iterator_ = Unatomic_base_vector::cbegin();
    return;
  }
  if (pos >= Unatomic_base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_vector::set_next: Position out of range.");
  }
  iterator_ = Unatomic_base_vector::cbegin() + pos;
}

template <typename T> T Atomic_ring_vector<T>::next() {
  static T buffer;
  static Const_iterator itr = Unatomic_base_vector::cbegin();

  std::ignore = prio_access_pending_.test_and_set(std::memory_order_release);
  const std::scoped_lock lck = Base_vector::get_lock();

  if (itr >= Unatomic_base_vector::cend()) {
    itr = Unatomic_base_vector::cbegin();
  }

  buffer = *itr++;

  iterator_.store(itr, std::memory_order_release);
  prio_access_pending_.clear();

  return buffer;
}

template <typename T>
inline typename Atomic_ring_vector<T>::Base_vector::Index
Atomic_ring_vector<T>::get_pos() const {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (Unatomic_base_vector::empty()) {
    return 0;
  }
  return static_cast<const Base_vector::Index>(
      iterator_.load(std::memory_order_acquire) -
      Unatomic_base_vector::cbegin());
}

// CRUD operations that handle iterator invalidation
template <typename T>
void Atomic_ring_vector<T>::assign(size_t n, const T& value) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  Unatomic_base_vector::assign(n, value);
  iterator_ = Unatomic_base_vector::cbegin();
}

template <typename T>
void Atomic_ring_vector<T>::assign(typename Base_vector::Initializer_list seq) {
  std::scoped_lock lck = get_lock();
  Unatomic_base_vector::assign(seq);
  iterator_ = Unatomic_base_vector::cbegin();
}

template <typename T>
void Atomic_ring_vector<T>::assign(const std::vector<T>& vec) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  Unatomic_base_vector::assign(vec.begin(), vec.end());
  iterator_ = Unatomic_base_vector::cbegin();
}

template <typename T> void Atomic_ring_vector<T>::push_back(const T& value) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  // Save current position as index
  typename Base_vector::Index current_pos = 0;
  if (!Unatomic_base_vector::empty()) {
    current_pos = static_cast<typename Base_vector::Index>(
        iterator_.load(std::memory_order_acquire) -
        Unatomic_base_vector::cbegin());
  }

  Unatomic_base_vector::push_back(value);

  // Restore iterator position (push_back may invalidate iterators if
  // reallocation occurs)
  if (!Unatomic_base_vector::empty()) {
    iterator_ = Unatomic_base_vector::cbegin() + current_pos;
  } else {
    iterator_ = Unatomic_base_vector::cbegin();
  }
}

template <typename T>
void Atomic_ring_vector<T>::insert(typename Base_vector::Index pos,
                                   const T& value) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (pos > Unatomic_base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_vector::insert: Position out of range.");
  }

  // Save current position as index
  typename Base_vector::Index current_pos = 0;
  if (!Unatomic_base_vector::empty()) {
    current_pos = static_cast<typename Base_vector::Index>(
        iterator_.load(std::memory_order_acquire) -
        Unatomic_base_vector::cbegin());
  }

  Unatomic_base_vector::insert(Unatomic_base_vector::begin() + pos, value);

  // Adjust iterator position if insertion was before current position
  if (pos <= current_pos) {
    current_pos++;
  }

  iterator_ = Unatomic_base_vector::cbegin() + current_pos;
}

template <typename T>
void Atomic_ring_vector<T>::erase(typename Base_vector::Index pos) {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  if (pos >= Unatomic_base_vector::size()) {
    throw std::out_of_range(
        "[ERROR] In Atomic_ring_vector::erase: Position out of range.");
  }

  // Save current position as index
  typename Base_vector::Index current_pos = 0;
  if (!Unatomic_base_vector::empty()) {
    current_pos = static_cast<typename Base_vector::Index>(
        iterator_.load(std::memory_order_acquire) -
        Unatomic_base_vector::cbegin());
  }

  Unatomic_base_vector::erase(Unatomic_base_vector::begin() + pos);

  // Adjust iterator position
  if (Unatomic_base_vector::empty()) {
    iterator_ = Unatomic_base_vector::cbegin();
  } else if (pos < current_pos) {
    // Erased element was before current position, decrement
    current_pos--;
    iterator_ = Unatomic_base_vector::cbegin() + current_pos;
  } else if (pos == current_pos) {
    // Erased element was at current position
    // If we're now past the end, wrap to beginning
    if (current_pos >= Unatomic_base_vector::size()) {
      current_pos = 0;
    }
    iterator_ = Unatomic_base_vector::cbegin() + current_pos;
  } else {
    // Erased element was after current position, no adjustment needed
    iterator_ = Unatomic_base_vector::cbegin() + current_pos;
  }
}

template <typename T> void Atomic_ring_vector<T>::clear() noexcept {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  Unatomic_base_vector::clear();
  iterator_ = Unatomic_base_vector::cbegin();
}

template <typename T>
std::vector<T> Atomic_ring_vector<T>::data() const noexcept {
  std::scoped_lock lck = Atomic_vector<T>::get_lock();
  return *this;
}

// Private
template <typename T>
const std::scoped_lock<std::mutex>
Atomic_ring_vector<T>::get_lock() const noexcept {
  while (prio_access_pending_.test_and_set(std::memory_order_acquire))
    ;
  prio_access_pending_.clear();
  return Base_vector::get_lock();
}

} // namespace container
} // namespace Micro_composer
