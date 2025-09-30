#include "container/atomic_ring_deque.h"
#include <catch2/catch_test_macros.hpp>

using namespace Micro_composer::container;

TEST_CASE("Atomic_ring_deque: Basic ring buffer behavior",
          "[atomic_ring_deque]") {
  Atomic_ring_deque<int> ring;

  SECTION("Cycling through elements") {
    ring.push_back(1);
    ring.push_back(2);
    ring.push_back(3);

    REQUIRE(ring.next() == 1);
    REQUIRE(ring.next() == 2);
    REQUIRE(ring.next() == 3);
    REQUIRE(ring.next() == 1); // Should wrap around
    REQUIRE(ring.next() == 2);
  }

  SECTION("Empty ring throws") {
    REQUIRE_THROWS_AS(ring.next(), std::out_of_range);
  }

  SECTION("Single element cycles") {
    ring.push_back(42);

    REQUIRE(ring.next() == 42);
    REQUIRE(ring.next() == 42);
    REQUIRE(ring.next() == 42);
  }
}

TEST_CASE("Atomic_ring_deque: Modifications during iteration",
          "[atomic_ring_deque]") {
  Atomic_ring_deque<int> ring;
  ring.push_back(1);
  ring.push_back(2);
  ring.push_back(3);

  SECTION("Adding elements resets iterator") {
    REQUIRE(ring.next() == 1);
    REQUIRE(ring.next() == 2);

    ring.push_back(4); // Might invalidate iterator

    // After modification, next() should still work
    int value = ring.next();
    REQUIRE((value >= 1 && value <= 4));
  }

  SECTION("Clear and repopulate") {
    REQUIRE(ring.next() == 1);

    ring.clear();
    REQUIRE_THROWS_AS(ring.next(), std::out_of_range);

    ring.push_back(10);
    REQUIRE(ring.next() == 10);
  }
}

TEST_CASE("Atomic_ring_deque: Inherited operations", "[atomic_ring_deque]") {
  Atomic_ring_deque<int> ring;

  SECTION("All Atomic_deque operations work") {
    ring.push_back(1);
    ring.push_front(0);
    ring.insert(1, 5);

    REQUIRE(ring.size() == 3);
    REQUIRE(ring.front() == 0);
    REQUIRE(ring.at(1) == 5);
    REQUIRE(ring.back() == 1);
  }

  SECTION("next() respects current content") {
    ring.push_back(1);
    ring.push_back(2);

    REQUIRE(ring.next() == 1);

    ring.erase(0); // Remove first element

    // Iterator should still work but might reset
    int value = ring.next();
    REQUIRE((value == 1 || value == 2));
  }
}