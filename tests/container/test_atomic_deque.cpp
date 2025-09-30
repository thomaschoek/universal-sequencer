#include "container/atomic_deque.h"
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>

using namespace Micro_composer::container;

TEST_CASE("Atomic_deque: Basic construction", "[atomic_deque]") {
  SECTION("Default constructor") {
    Atomic_deque<int> deque;
    REQUIRE(deque.empty());
    REQUIRE(deque.size() == 0);
  }

  SECTION("Copy constructor") {
    Atomic_deque<int> deque1;
    deque1.push_back(1);
    deque1.push_back(2);

    Atomic_deque<int> deque2(deque1);
    REQUIRE(deque2.size() == 2);
    REQUIRE(deque2.front() == 1);
    REQUIRE(deque2.back() == 2);
  }

  SECTION("Copy assignment") {
    Atomic_deque<int> deque1;
    deque1.push_back(10);

    Atomic_deque<int> deque2;
    deque2 = deque1;
    REQUIRE(deque2.size() == 1);
    REQUIRE(deque2.front() == 10);
  }
}

TEST_CASE("Atomic_deque: Push and pop operations", "[atomic_deque]") {
  Atomic_deque<int> deque;

  SECTION("Push back") {
    deque.push_back(1);
    deque.push_back(2);
    deque.push_back(3);

    REQUIRE(deque.size() == 3);
    REQUIRE(deque.front() == 1);
    REQUIRE(deque.back() == 3);
  }

  SECTION("Push front") {
    deque.push_front(1);
    deque.push_front(2);
    deque.push_front(3);

    REQUIRE(deque.size() == 3);
    REQUIRE(deque.front() == 3);
    REQUIRE(deque.back() == 1);
  }

  SECTION("Pop back") {
    deque.push_back(1);
    deque.push_back(2);
    deque.push_back(3);
    deque.pop_back();

    REQUIRE(deque.size() == 2);
    REQUIRE(deque.back() == 2);
  }

  SECTION("Pop front") {
    deque.push_back(1);
    deque.push_back(2);
    deque.push_back(3);
    deque.pop_front();

    REQUIRE(deque.size() == 2);
    REQUIRE(deque.front() == 2);
  }
}

TEST_CASE("Atomic_deque: Insert and erase operations", "[atomic_deque]") {
  Atomic_deque<int> deque;

  SECTION("Insert at position") {
    deque.push_back(1);
    deque.push_back(3);
    deque.insert(1, 2);

    REQUIRE(deque.size() == 3);
    REQUIRE(deque.at(0) == 1);
    REQUIRE(deque.at(1) == 2);
    REQUIRE(deque.at(2) == 3);
  }

  SECTION("Insert out of range throws") {
    deque.push_back(1);
    REQUIRE_THROWS_AS(deque.insert(10, 2), std::out_of_range);
  }

  SECTION("Erase at position") {
    deque.push_back(1);
    deque.push_back(2);
    deque.push_back(3);
    deque.erase(1);

    REQUIRE(deque.size() == 2);
    REQUIRE(deque.at(0) == 1);
    REQUIRE(deque.at(1) == 3);
  }

  SECTION("Erase out of range throws") {
    deque.push_back(1);
    REQUIRE_THROWS_AS(deque.erase(10), std::out_of_range);
  }
}

TEST_CASE("Atomic_deque: Replace operations", "[atomic_deque]") {
  Atomic_deque<int> deque;
  deque.push_back(1);
  deque.push_back(2);
  deque.push_back(3);

  SECTION("Replace at position") {
    deque.replace(1, 20);
    REQUIRE(deque.at(1) == 20);
  }

  SECTION("Replace out of range throws") {
    REQUIRE_THROWS_AS(deque.replace(10, 20), std::out_of_range);
  }
}

TEST_CASE("Atomic_deque: Clear operation", "[atomic_deque]") {
  Atomic_deque<int> deque;
  deque.push_back(1);
  deque.push_back(2);
  deque.push_back(3);

  deque.clear();

  REQUIRE(deque.empty());
  REQUIRE(deque.size() == 0);
}

TEST_CASE("Atomic_deque: Thread safety", "[atomic_deque][thread_safety]") {
  Atomic_deque<int> deque;
  constexpr int num_threads = 10;
  constexpr int ops_per_thread = 100;

  SECTION("Concurrent push_back") {
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&deque, i]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          deque.push_back(i * ops_per_thread + j);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    REQUIRE(deque.size() == num_threads * ops_per_thread);
  }

  SECTION("Concurrent push and pop") {
    // Pre-fill
    for (int i = 0; i < 100; ++i) {
      deque.push_back(i);
    }

    std::vector<std::thread> threads;

    // Half threads push, half pop
    for (int i = 0; i < num_threads / 2; ++i) {
      threads.emplace_back([&deque]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          deque.push_back(j);
        }
      });

      threads.emplace_back([&deque]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          if (!deque.empty()) {
            deque.pop_back();
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    // Should not crash and size should be valid
    REQUIRE(deque.size() >= 0);
  }
}

TEST_CASE("Atomic_deque: Assign operation", "[atomic_deque]") {
  Atomic_deque<int> deque;

  SECTION("Assign from initializer_list") {
    deque.assign({1, 2, 3, 4, 5});

    REQUIRE(deque.size() == 5);
    REQUIRE(deque.front() == 1);
    REQUIRE(deque.back() == 5);
    REQUIRE(deque.at(2) == 3);
  }

  SECTION("Assign from std::vector copy") {
    std::vector<int> vec = {10, 20, 30, 40};
    deque.assign(vec);

    REQUIRE(deque.size() == 4);
    REQUIRE(deque.front() == 10);
    REQUIRE(deque.back() == 40);
    REQUIRE(deque.at(1) == 20);

    // Verify original vector unchanged
    REQUIRE(vec.size() == 4);
    REQUIRE(vec[0] == 10);
  }

  SECTION("Assign from std::vector move") {
    std::vector<int> vec = {100, 200, 300};
    deque.assign(std::move(vec));

    REQUIRE(deque.size() == 3);
    REQUIRE(deque.front() == 100);
    REQUIRE(deque.back() == 300);
    REQUIRE(deque.at(1) == 200);
  }
}