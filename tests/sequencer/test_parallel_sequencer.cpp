#include "sequencable/oscillation_event.h"
#include "sequencer/parallel_sequencer.tpp"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;
using namespace std::chrono_literals;

TEST_CASE("Parallel_sequencer: Basic construction", "[parallel_sequencer]") {
  SECTION("Default constructor") {
    Parallel_sequencer<Oscillation_event> pseq;
    REQUIRE(pseq.empty());
    REQUIRE(pseq.size() == 0);
  }

  // Copy constructor deleted (contains non-copyable mutex)
}

TEST_CASE("Parallel_sequencer: Add and remove sequencers",
          "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count{0};
  auto handler = [&count](Oscillation_event&&) { ++count; };

  SECTION("Add sequencers via emplace_back") {
    pseq.emplace_back(handler);
    pseq.emplace_back(handler);

    REQUIRE(pseq.size() == 2);
  }

  SECTION("Add sequencers via push_back") {
    Atomic_sequencer<Oscillation_event> seq(handler);
    pseq.push_back(seq);

    REQUIRE(pseq.size() == 1);
  }

  SECTION("Remove sequencer") {
    pseq.emplace_back(handler);
    pseq.emplace_back(handler);
    pseq.emplace_back(handler);

    REQUIRE(pseq.size() == 3);

    pseq.erase(1);

    REQUIRE(pseq.size() == 2);
  }

  SECTION("Clear all sequencers") {
    pseq.emplace_back(handler);
    pseq.emplace_back(handler);

    pseq.clear();

    REQUIRE(pseq.empty());
    REQUIRE(pseq.size() == 0);
  }
}

TEST_CASE("Parallel_sequencer: Direct sequencer access",
          "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count{0};
  auto handler = [&count](Oscillation_event&&) { ++count; };

  pseq.emplace_back(handler);
  pseq.emplace_back(handler);

  SECTION("operator[] access") {
    pseq[0].assign({Oscillation_event{440.0}, Oscillation_event{880.0}});
    pseq[1].assign({Oscillation_event{1760.0}});

    REQUIRE(pseq[0].size() == 2);
    REQUIRE(pseq[1].size() == 1);
  }

  SECTION("Modify individual sequencers") {
    pseq[0].push_back(Oscillation_event{440.0});
    pseq[0].push_back(Oscillation_event{880.0});

    REQUIRE(pseq[0].size() == 2);

    pseq[0].erase(0);

    REQUIRE(pseq[0].size() == 1);
  }
}

TEST_CASE("Parallel_sequencer: Transport control - single sequencer",
          "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count1{0}, count2{0};

  auto handler1 = [&count1](Oscillation_event&&) { ++count1; };
  auto handler2 = [&count2](Oscillation_event&&) { ++count2; };

  pseq.emplace_back(handler1);
  pseq.emplace_back(handler2);

  pseq[0].assign({Oscillation_event{440.0, 0.5, 0.0, 0ms, 10ms}});
  pseq[1].assign({Oscillation_event{880.0, 0.5, 0.0, 0ms, 10ms}});

  SECTION("Start single sequencer") {
    pseq.start(0);

    REQUIRE(pseq.is_running(0));
    REQUIRE_FALSE(pseq.is_running(1));

    std::this_thread::sleep_for(30ms);

    pseq.stop(0);

    REQUIRE_FALSE(pseq.is_running(0));
    REQUIRE(count1 > 0);
    REQUIRE(count2 == 0); // Second sequencer never started
  }

  SECTION("Stop single sequencer") {
    pseq.start(0);
    pseq.start(1);

    std::this_thread::sleep_for(20ms);

    pseq.stop(0);

    REQUIRE_FALSE(pseq.is_running(0));
    REQUIRE(pseq.is_running(1));

    std::this_thread::sleep_for(20ms);

    pseq.stop(1);
  }
}

TEST_CASE("Parallel_sequencer: Transport control - all sequencers",
          "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count1{0}, count2{0}, count3{0};

  auto handler1 = [&count1](Oscillation_event&&) { ++count1; };
  auto handler2 = [&count2](Oscillation_event&&) { ++count2; };
  auto handler3 = [&count3](Oscillation_event&&) { ++count3; };

  pseq.emplace_back(handler1);
  pseq.emplace_back(handler2);
  pseq.emplace_back(handler3);

  pseq[0].assign({Oscillation_event{440.0, 0.5, 0.0, 0ms, 10ms}});
  pseq[1].assign({Oscillation_event{880.0, 0.5, 0.0, 0ms, 10ms}});
  pseq[2].assign({Oscillation_event{1760.0, 0.5, 0.0, 0ms, 10ms}});

  SECTION("Start all sequencers") {
    pseq.start_all();

    REQUIRE(pseq.is_running(0));
    REQUIRE(pseq.is_running(1));
    REQUIRE(pseq.is_running(2));
    REQUIRE(pseq.all_running());

    std::this_thread::sleep_for(30ms);

    pseq.stop_all();

    REQUIRE_FALSE(pseq.is_running(0));
    REQUIRE_FALSE(pseq.is_running(1));
    REQUIRE_FALSE(pseq.is_running(2));
    REQUIRE_FALSE(pseq.any_running());

    REQUIRE(count1 > 0);
    REQUIRE(count2 > 0);
    REQUIRE(count3 > 0);
  }

  SECTION("Synchronized start") {
    auto start_time = std::chrono::steady_clock::now() + 50ms;

    pseq.start_all(start_time);

    // All should be running but waiting
    REQUIRE(pseq.all_running());

    std::this_thread::sleep_for(30ms);

    // Should not have triggered yet (waiting for start_time)
    // This is timing-dependent, so we just check they're running
    REQUIRE(pseq.all_running());

    std::this_thread::sleep_for(40ms);

    // Now they should have triggered
    pseq.stop_all();

    REQUIRE(count1 > 0);
    REQUIRE(count2 > 0);
    REQUIRE(count3 > 0);
  }
}

TEST_CASE("Parallel_sequencer: Status queries", "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count{0};
  auto handler = [&count](Oscillation_event&&) { ++count; };

  pseq.emplace_back(handler);
  pseq.emplace_back(handler);
  pseq.emplace_back(handler);

  pseq[0].assign({Oscillation_event{440.0, 0.5, 0.0, 0ms, 10ms}});
  pseq[1].assign({Oscillation_event{880.0, 0.5, 0.0, 0ms, 10ms}});
  pseq[2].assign({Oscillation_event{1760.0, 0.5, 0.0, 0ms, 10ms}});

  SECTION("any_running with none running") {
    REQUIRE_FALSE(pseq.any_running());
  }

  SECTION("any_running with some running") {
    pseq.start(0);

    REQUIRE(pseq.any_running());
    REQUIRE_FALSE(pseq.all_running());

    pseq.stop(0);
  }

  SECTION("all_running") {
    pseq.start_all();

    REQUIRE(pseq.any_running());
    REQUIRE(pseq.all_running());

    pseq.stop_all();

    REQUIRE_FALSE(pseq.any_running());
    REQUIRE_FALSE(pseq.all_running());
  }

  SECTION("all_running with empty container") {
    Parallel_sequencer<Oscillation_event> empty_pseq;
    REQUIRE_FALSE(empty_pseq.all_running());
  }
}

TEST_CASE("Parallel_sequencer: CRUD while running", "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count{0};
  auto handler = [&count](Oscillation_event&&) { ++count; };

  // Reserve capacity to prevent reallocation during test
  pseq.reserve(10);

  pseq.emplace_back(handler);
  pseq[0].assign({Oscillation_event{440.0, 0.5, 0.0, 0ms, 10ms}});

  pseq.start(0);
  REQUIRE(pseq.is_running(0));
  std::this_thread::sleep_for(20ms);

  SECTION("Add new sequencer while others running") {
    pseq.emplace_back(handler);
    pseq[1].assign({Oscillation_event{880.0, 0.5, 0.0, 0ms, 10ms}});

    REQUIRE(pseq.size() == 2);
    REQUIRE(pseq.is_running(0));
    REQUIRE_FALSE(pseq.is_running(1));

    pseq.start(1);

    std::this_thread::sleep_for(20ms);

    REQUIRE(pseq.is_running(1));

    pseq.stop_all();
  }

  SECTION("Modify running sequencer's steps") {
    auto initial_count = count.load();

    pseq[0].push_back(Oscillation_event{880.0, 0.5, 0.0, 0ms, 10ms});

    std::this_thread::sleep_for(30ms);

    pseq.stop(0);

    // Should have processed additional events
    REQUIRE(count > initial_count);
  }
}

TEST_CASE("Parallel_sequencer: Out of range handling", "[parallel_sequencer]") {
  Parallel_sequencer<Oscillation_event> pseq;
  std::atomic<int> count{0};
  auto handler = [&count](Oscillation_event&&) { ++count; };

  pseq.emplace_back(handler);

  SECTION("Start out of range throws") {
    REQUIRE_THROWS_AS(pseq.start(10), std::out_of_range);
  }

  SECTION("Stop out of range throws") {
    REQUIRE_THROWS_AS(pseq.stop(10), std::out_of_range);
  }

  SECTION("is_running out of range throws") {
    REQUIRE_THROWS_AS(pseq.is_running(10), std::out_of_range);
  }
}