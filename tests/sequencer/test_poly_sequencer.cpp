#include "sequencer/poly_sequencer.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::sequencer;

// Test event type that satisfies Has_duration concept
struct Poly_test_event {
  using Duration = std::chrono::steady_clock::duration;
  Duration dur;

  Poly_test_event() : dur(std::chrono::milliseconds(100)) {}
  explicit Poly_test_event(Duration d) : dur(d) {}

  // Conversion operator to Duration for use in Sequencer
  operator Duration() const { return dur; }
};

// Verify Poly_test_event satisfies Has_duration concept
static_assert(Sequencable<Poly_test_event>,
              "Poly_test_event does not satisfy Has_duration concept");

TEST_CASE("Poly_sequencer construction", "[poly_sequencer]") {
  SECTION("Default constructor creates empty poly_sequencer") {
    Poly_sequencer<Poly_test_event> poly;
    REQUIRE(poly.empty());
    REQUIRE(poly.size() == 0);
  }

  SECTION("Construction from vector of vectors") {
    std::vector<std::vector<Poly_test_event>> sequences = {
        {Poly_test_event(std::chrono::milliseconds(100)),
         Poly_test_event(std::chrono::milliseconds(200))},
        {Poly_test_event(std::chrono::milliseconds(150)),
         Poly_test_event(std::chrono::milliseconds(250))}};

    Poly_sequencer<Poly_test_event> poly(sequences);
    REQUIRE(poly.size() == 2);
  }

  SECTION("Construction from rvalue vector of vectors") {
    std::vector<std::vector<Poly_test_event>> sequences = {
        {Poly_test_event(std::chrono::milliseconds(100)),
         Poly_test_event(std::chrono::milliseconds(200))},
        {Poly_test_event(std::chrono::milliseconds(150)),
         Poly_test_event(std::chrono::milliseconds(250))}};

    Poly_sequencer<Poly_test_event> poly(std::move(sequences));
    REQUIRE(poly.size() == 2);
  }
}

TEST_CASE("Poly_sequencer basic operations", "[poly_sequencer]") {
  Poly_sequencer<Poly_test_event> poly;

  SECTION("Can add sequencers") {
    Sequencer<Poly_test_event> seq1(
        {Poly_test_event(std::chrono::milliseconds(100))});
    Sequencer<Poly_test_event> seq2(
        {Poly_test_event(std::chrono::milliseconds(200))});

    poly.push_back(std::move(seq1));
    poly.push_back(std::move(seq2));

    REQUIRE(poly.size() == 2);
  }

  SECTION("Can access sequencers") {
    Sequencer<Poly_test_event> seq(
        {Poly_test_event(std::chrono::milliseconds(100)),
         Poly_test_event(std::chrono::milliseconds(200))});
    poly.push_back(std::move(seq));

    REQUIRE(poly.size() == 1);
    REQUIRE(poly[0].size() == 2);
  }
}

TEST_CASE("Poly_sequencer state queries", "[poly_sequencer]") {
  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50))},
      {Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50))}};

  Poly_sequencer<Poly_test_event> poly(sequences);

  SECTION("Initially no sequencers are scheduling") {
    REQUIRE_FALSE(poly.any_scheduling());
    REQUIRE_FALSE(poly.all_scheduling());
    REQUIRE_FALSE(poly.is_scheduling(0));
    REQUIRE_FALSE(poly.is_scheduling(1));
  }

  SECTION("is_scheduling throws for out of range index") {
    REQUIRE_THROWS_AS(poly.is_scheduling(999), std::out_of_range);
  }
}

TEST_CASE("Poly_sequencer transport control", "[poly_sequencer]") {
  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50))},
      {Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50))}};

  Poly_sequencer<Poly_test_event> poly(sequences);

  SECTION("start throws for out of range index") {
    auto start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(100);
    REQUIRE_THROWS_AS(poly.start(999, start_time), std::out_of_range);
  }

  SECTION("pause throws for out of range index") {
    auto pause_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(100);
    REQUIRE_THROWS_AS(poly.pause(999, pause_time), std::out_of_range);
  }

  SECTION("reset throws for out of range index") {
    auto reset_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(100);
    REQUIRE_THROWS_AS(poly.reset(999, reset_time), std::out_of_range);
  }

  SECTION("set_next throws for out of range index") {
    REQUIRE_THROWS_AS(poly.set_next(999, 0), std::out_of_range);
  }

  SECTION("set_next_all for all sequencers") {
    // Should not throw
    poly.set_next_all(1);
    REQUIRE(true);
  }

  SECTION("pause_all on stopped sequencers is safe") {
    auto pause_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.pause_all(pause_time);
    REQUIRE_FALSE(poly.any_scheduling());
  }

  SECTION("reset_all on stopped sequencers is safe") {
    auto reset_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.reset_all(reset_time, 0);
    REQUIRE_FALSE(poly.any_scheduling());
  }
}

TEST_CASE("Poly_sequencer empty container", "[poly_sequencer]") {
  Poly_sequencer<Poly_test_event> poly;

  SECTION("any_scheduling returns false for empty") {
    REQUIRE_FALSE(poly.any_scheduling());
  }

  SECTION("all_scheduling returns false for empty") {
    REQUIRE_FALSE(poly.all_scheduling());
  }

  SECTION("pause_all is safe on empty") {
    auto pause_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.pause_all(pause_time);
    REQUIRE(poly.empty());
  }

  SECTION("set_next_all is safe on empty") {
    poly.set_next_all(0);
    REQUIRE(poly.empty());
  }
}

TEST_CASE("Poly_sequencer with mixed sequencer sizes", "[poly_sequencer]") {
  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50))}, // 1 event
      {Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50)),
       Poly_test_event(std::chrono::milliseconds(50))} // 3 events
  };

  Poly_sequencer<Poly_test_event> poly(sequences);

  SECTION("Sequencers have different sizes") {
    REQUIRE(poly.size() == 2);
    REQUIRE(poly[0].size() == 1);
    REQUIRE(poly[1].size() == 3);
  }

  SECTION("Can set position independently") {
    poly.set_next(0, 0);
    poly.set_next(1, 2);
    // If we had a way to query position, we'd verify here
    REQUIRE(true);
  }
}

TEST_CASE("Poly_sequencer concurrent access", "[poly_sequencer][concurrency]") {
  std::vector<std::vector<Poly_test_event>> sequences;
  for (int i = 0; i < 5; ++i) {
    sequences.push_back({Poly_test_event(std::chrono::milliseconds(50)),
                         Poly_test_event(std::chrono::milliseconds(50))});
  }

  Poly_sequencer<Poly_test_event> poly(sequences);

  SECTION("Concurrent state queries are safe") {
    std::vector<std::thread> threads;
    std::atomic<int> query_count{0};

    for (int i = 0; i < 5; ++i) {
      threads.emplace_back([&poly, &query_count]() {
        for (int j = 0; j < 10; ++j) {
          poly.any_scheduling();
          poly.all_scheduling();
          ++query_count;
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    REQUIRE(query_count == 50);
  }
}
