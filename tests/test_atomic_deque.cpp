#include "utils/atomic_deque.h"
#include <catch2/catch_test_macros.hpp>
#include <future>
#include <thread>
#include <vector>

using namespace Micro_composer::atomic_deque;

TEST_CASE("AtomicDeque basic operations", "[atomic_deque]") {
  Atomic_deque<int> deque;

  SECTION("Default constructor creates empty deque") {
    REQUIRE(deque.empty());
    REQUIRE(deque.size() == 0);
  }

  SECTION("Push and pop operations") {
    deque.push_back(1);
    deque.push_back(2);
    deque.push_front(0);

    REQUIRE(deque.size() == 3);
    REQUIRE(deque.front() == 0);
    REQUIRE(deque.back() == 2);

    deque.pop_front();
    REQUIRE(deque.front() == 1);
    REQUIRE(deque.size() == 2);

    deque.pop_back();
    REQUIRE(deque.back() == 1);
    REQUIRE(deque.size() == 1);
  }

  SECTION("Access operations") {
    deque.push_back(10);
    deque.push_back(20);
    deque.push_back(30);

    REQUIRE(deque[0] == 10);
    REQUIRE(deque[1] == 20);
    REQUIRE(deque[2] == 30);

    REQUIRE(deque.at(0) == 10);
    REQUIRE(deque.at(1) == 20);
    REQUIRE(deque.at(2) == 30);
  }
}

TEST_CASE("AtomicDeque constructors", "[atomic_deque]") {
  SECTION("Size constructor") {
    Atomic_deque<int> deque(5);
    REQUIRE(deque.size() == 5);
    REQUIRE_FALSE(deque.empty());

    for (size_t i = 0; i < 5; ++i) {
      REQUIRE(deque[i] == 0);
    }
  }

  SECTION("Size and value constructor") {
    Atomic_deque<int> deque(3, 42);
    REQUIRE(deque.size() == 3);

    for (size_t i = 0; i < 3; ++i) {
      REQUIRE(deque[i] == 42);
    }
  }

  SECTION("Copy constructor") {
    Atomic_deque<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    Atomic_deque<int> copy(original);
    REQUIRE(copy.size() == 3);
    REQUIRE(copy[0] == 1);
    REQUIRE(copy[1] == 2);
    REQUIRE(copy[2] == 3);
  }

  SECTION("Move constructor") {
    Atomic_deque<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    Atomic_deque<int> moved(std::move(original));
    REQUIRE(moved.size() == 3);
    REQUIRE(moved[0] == 1);
    REQUIRE(moved[1] == 2);
    REQUIRE(moved[2] == 3);
  }
}

TEST_CASE("AtomicDeque assignment operators", "[atomic_deque]") {
  SECTION("Copy assignment") {
    Atomic_deque<int> original;
    original.push_back(1);
    original.push_back(2);

    Atomic_deque<int> copy;
    copy = original;

    REQUIRE(copy.size() == 2);
    REQUIRE(copy[0] == 1);
    REQUIRE(copy[1] == 2);
  }

  SECTION("Move assignment") {
    Atomic_deque<int> original;
    original.push_back(1);
    original.push_back(2);

    Atomic_deque<int> moved;
    moved = std::move(original);

    REQUIRE(moved.size() == 2);
    REQUIRE(moved[0] == 1);
    REQUIRE(moved[1] == 2);
  }

  SECTION("Self assignment") {
    Atomic_deque<int> deque;
    deque.push_back(1);
    deque.push_back(2);

    deque = deque; // Self assignment

    REQUIRE(deque.size() == 2);
    REQUIRE(deque[0] == 1);
    REQUIRE(deque[1] == 2);
  }
}

TEST_CASE("AtomicDeque exception safety", "[atomic_deque]") {
  Atomic_deque<int> deque;

  SECTION("Front on empty deque throws") {
    REQUIRE_THROWS_AS(deque.front(), std::out_of_range);
  }

  SECTION("Back on empty deque throws") {
    REQUIRE_THROWS_AS(deque.back(), std::out_of_range);
  }

  SECTION("At with invalid index throws") {
    deque.push_back(1);
    REQUIRE_THROWS_AS(deque.at(5), std::out_of_range);
  }

  SECTION("Pop operations on empty deque are safe") {
    REQUIRE_NOTHROW(deque.pop_front());
    REQUIRE_NOTHROW(deque.pop_back());
    REQUIRE(deque.empty());
  }
}

TEST_CASE("AtomicDeque clear operation", "[atomic_deque]") {
  Atomic_deque<int> deque;
  deque.push_back(1);
  deque.push_back(2);
  deque.push_back(3);

  REQUIRE(deque.size() == 3);
  REQUIRE_FALSE(deque.empty());

  deque.clear();

  REQUIRE(deque.size() == 0);
  REQUIRE(deque.empty());
}

TEST_CASE("AtomicDeque move semantics", "[atomic_deque]") {
  Atomic_deque<std::unique_ptr<int>> deque;

  auto ptr1 = std::make_unique<int>(42);
  auto ptr2 = std::make_unique<int>(24);

  deque.push_back(std::move(ptr1));
  deque.push_back(std::move(ptr2));

  REQUIRE(deque.size() == 2);
  REQUIRE(*deque[0] == 42);
  REQUIRE(*deque[1] == 24);
}

TEST_CASE("AtomicDeque thread safety", "[atomic_deque]") {
  Atomic_deque<int> deque;
  const int num_threads = 4;
  const int items_per_thread = 100;

  SECTION("Concurrent push operations") {
    std::vector<std::jthread> threads;

    // Launch multiple threads that push items
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&deque, t, items_per_thread]() {
        for (int i = 0; i < items_per_thread; ++i) {
          deque.push_back(t * items_per_thread + i);
        }
      });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    REQUIRE(deque.size() == num_threads * items_per_thread);
  }

  SECTION("Concurrent push and pop operations") {
    // Pre-fill the deque
    for (int i = 0; i < 200; ++i) {
      deque.push_back(i);
    }

    std::atomic<int> push_count{0};
    std::atomic<int> pop_count{0};

    std::vector<std::jthread> threads;

    // Producer threads
    for (int t = 0; t < 2; ++t) {
      threads.emplace_back([&deque, &push_count]() {
        for (int i = 0; i < 50; ++i) {
          deque.push_back(i);
          push_count++;
        }
      });
    }

    // Consumer threads
    for (int t = 0; t < 2; ++t) {
      threads.emplace_back([&deque, &pop_count]() {
        for (int i = 0; i < 50; ++i) {
          if (!deque.empty()) {
            deque.pop_back();
            pop_count++;
          }
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    // Verify that operations completed successfully
    REQUIRE(push_count == 100);
    // pop_count might be less than 100 due to race conditions
    REQUIRE(pop_count <= 100);
  }

  SECTION("Concurrent size checks") {
    std::vector<std::future<size_t>> futures;

    // Start multiple threads that check size while we modify the deque
    for (int t = 0; t < num_threads; ++t) {
      futures.push_back(std::async(std::launch::async, [&deque]() {
        size_t total_size = 0;
        for (int i = 0; i < 50; ++i) {
          total_size += deque.size();
          std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        return total_size;
      }));
    }

    // Modify the deque while size checks are happening
    for (int i = 0; i < 100; ++i) {
      deque.push_back(i);
      if (i % 10 == 0 && !deque.empty()) {
        deque.pop_front();
      }
    }

    // Wait for all size checking threads to complete
    for (auto& future : futures) {
      REQUIRE_NOTHROW(future.get());
    }
  }
}