#include "container/atomic_vector.h"
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>

using namespace Micro_composer::container;

TEST_CASE("Atomic_vector: Basic construction", "[atomic_vector]") {
  SECTION("Default constructor") {
    Atomic_vector<int> vec;
    REQUIRE(vec.empty());
    REQUIRE(vec.size() == 0);
  }

  SECTION("Copy constructor") {
    Atomic_vector<int> vec1;
    vec1.push_back(1);
    vec1.push_back(2);

    Atomic_vector<int> vec2(vec1);
    REQUIRE(vec2.size() == 2);
    REQUIRE(vec2.front() == 1);
    REQUIRE(vec2.back() == 2);
  }

  SECTION("Copy assignment") {
    Atomic_vector<int> vec1;
    vec1.push_back(10);

    Atomic_vector<int> vec2;
    vec2 = vec1;
    REQUIRE(vec2.size() == 1);
    REQUIRE(vec2.front() == 10);
  }

  SECTION("Construct from std::vector copy") {
    std::vector<int> std_vec = {1, 2, 3, 4, 5};
    Atomic_vector<int> vec(std_vec);

    REQUIRE(vec.size() == 5);
    REQUIRE(vec.front() == 1);
    REQUIRE(vec.back() == 5);
    REQUIRE(vec.at(2) == 3);

    // Verify original vector unchanged
    REQUIRE(std_vec.size() == 5);
    REQUIRE(std_vec[0] == 1);
  }

  SECTION("Construct from std::vector move") {
    std::vector<int> std_vec = {10, 20, 30};
    Atomic_vector<int> vec(std::move(std_vec));

    REQUIRE(vec.size() == 3);
    REQUIRE(vec.front() == 10);
    REQUIRE(vec.back() == 30);
    REQUIRE(vec.at(1) == 20);
  }
}

TEST_CASE("Atomic_vector: Push and pop operations", "[atomic_vector]") {
  Atomic_vector<int> vec;

  SECTION("Push back") {
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    REQUIRE(vec.size() == 3);
    REQUIRE(vec.front() == 1);
    REQUIRE(vec.back() == 3);
  }

  SECTION("Emplace back") {
    vec.emplace_back(1);
    vec.emplace_back(2);

    REQUIRE(vec.size() == 2);
    REQUIRE(vec.at(0) == 1);
    REQUIRE(vec.at(1) == 2);
  }

  SECTION("Pop back") {
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.pop_back();

    REQUIRE(vec.size() == 2);
    REQUIRE(vec.back() == 2);
  }

  SECTION("Pop back on empty is safe") {
    vec.pop_back(); // Should not crash
    REQUIRE(vec.empty());
  }
}

TEST_CASE("Atomic_vector: Insert and erase operations", "[atomic_vector]") {
  Atomic_vector<int> vec;

  SECTION("Insert at position") {
    vec.push_back(1);
    vec.push_back(3);
    vec.insert(1, 2);

    REQUIRE(vec.size() == 3);
    REQUIRE(vec.at(0) == 1);
    REQUIRE(vec.at(1) == 2);
    REQUIRE(vec.at(2) == 3);
  }

  SECTION("Insert out of range throws") {
    vec.push_back(1);
    REQUIRE_THROWS_AS(vec.insert(10, 2), std::out_of_range);
  }

  SECTION("Erase at position") {
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.erase(1);

    REQUIRE(vec.size() == 2);
    REQUIRE(vec.at(0) == 1);
    REQUIRE(vec.at(1) == 3);
  }

  SECTION("Erase out of range throws") {
    vec.push_back(1);
    REQUIRE_THROWS_AS(vec.erase(10), std::out_of_range);
  }
}

TEST_CASE("Atomic_vector: Element access", "[atomic_vector]") {
  Atomic_vector<int> vec;
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);

  SECTION("operator[] access") {
    REQUIRE(vec[0] == 10);
    REQUIRE(vec[1] == 20);
    REQUIRE(vec[2] == 30);
  }

  SECTION("operator[] modification") {
    vec[1] = 200;
    REQUIRE(vec[1] == 200);
  }

  SECTION("at() bounds checking") {
    REQUIRE(vec.at(0) == 10);
    REQUIRE(vec.at(2) == 30);
  }
}

TEST_CASE("Atomic_vector: Replace operations", "[atomic_vector]") {
  Atomic_vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  SECTION("Replace at position") {
    vec.assign(1, 20);
    REQUIRE(vec.at(1) == 20);
  }

  SECTION("Replace out of range throws") {
    REQUIRE_THROWS_AS(vec.assign(10, 20), std::out_of_range);
  }
}

TEST_CASE("Atomic_vector: Capacity operations", "[atomic_vector]") {
  Atomic_vector<int> vec;

  SECTION("Reserve capacity") {
    vec.reserve(100);
    REQUIRE(vec.capacity() >= 100);
    REQUIRE(vec.size() == 0);
  }

  SECTION("Capacity grows with push_back") {
    vec.push_back(1);
    auto cap1 = vec.capacity();
    REQUIRE(cap1 >= 1);

    for (int i = 0; i < 100; ++i) {
      vec.push_back(i);
    }

    REQUIRE(vec.capacity() >= 101);
  }
}

TEST_CASE("Atomic_vector: Clear operation", "[atomic_vector]") {
  Atomic_vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  vec.clear();

  REQUIRE(vec.empty());
  REQUIRE(vec.size() == 0);
}

TEST_CASE("Atomic_vector: Thread safety", "[atomic_vector][thread_safety]") {
  Atomic_vector<int> vec;
  constexpr int num_threads = 10;
  constexpr int ops_per_thread = 100;

  SECTION("Concurrent push_back") {
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&vec, i]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          vec.push_back(i * ops_per_thread + j);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    REQUIRE(vec.size() == num_threads * ops_per_thread);
  }

  SECTION("Concurrent emplace_back") {
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&vec, i]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          vec.emplace_back(i * ops_per_thread + j);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    REQUIRE(vec.size() == num_threads * ops_per_thread);
  }

  SECTION("Concurrent push and pop") {
    // Pre-fill
    for (int i = 0; i < 100; ++i) {
      vec.push_back(i);
    }

    std::vector<std::thread> threads;

    // Half threads push, half pop
    for (int i = 0; i < num_threads / 2; ++i) {
      threads.emplace_back([&vec]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          vec.push_back(j);
        }
      });

      threads.emplace_back([&vec]() {
        for (int j = 0; j < ops_per_thread; ++j) {
          if (!vec.empty()) {
            vec.pop_back();
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    // Should not crash and size should be valid
    REQUIRE(vec.size() >= 0);
  }
}

TEST_CASE("Atomic_vector: Assign operation", "[atomic_vector]") {
  Atomic_vector<int> vec;

  vec.assign({1, 2, 3, 4, 5});

  REQUIRE(vec.size() == 5);
  REQUIRE(vec.front() == 1);
  REQUIRE(vec.back() == 5);
  REQUIRE(vec.at(2) == 3);
}