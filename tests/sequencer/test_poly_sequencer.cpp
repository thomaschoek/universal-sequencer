#include "sequencable/mutable_event.h"
#include "sequencer/poly_sequencer_template.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;

namespace Micro_composer {
namespace tests {
namespace poly_sequencer_tests {

// Test event type that satisfies Mut_seq_event concept
struct Poly_test_event : public Mutable_event {
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;

  Time_point scheduled_time{};
  Duration duration{std::chrono::milliseconds(50)};
  int id{0};
  bool enabled{true};

  Poly_test_event() = default;
  explicit Poly_test_event(Duration d, int event_id = 0)
      : duration(d), id(event_id) {}

  void update(const Poly_test_event& other) {
    duration = other.duration;
    id = other.id;
  }

  void update(Duration d, int event_id = 0) {
    duration = d;
    id = event_id;
  }
};

static_assert(Mut_seq_event<Poly_test_event>,
              "Poly_test_event does not satisfy Mut_seq_event concept");

// Event capture helper
class Poly_event_capture {
public:
  std::vector<std::vector<Poly_test_event>> events;
  mutable std::mutex mutex;
  std::condition_variable cv;

  void capture(int seq_idx, Poly_test_event&& evt) {
    std::lock_guard<std::mutex> lock(mutex);
    if (seq_idx >= static_cast<int>(events.size())) {
      events.resize(seq_idx + 1);
    }
    events[seq_idx].push_back(std::move(evt));
    cv.notify_one();
  }

  std::function<void(Poly_test_event&&)> make_handler(int seq_idx) {
    return [this, seq_idx](Poly_test_event&& evt) {
      capture(seq_idx, std::move(evt));
    };
  }

  size_t total_events() const {
    std::lock_guard<std::mutex> lock(mutex);
    size_t total = 0;
    for (const auto& vec : events) {
      total += vec.size();
    }
    return total;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex);
    events.clear();
  }
};

TEST_CASE("Poly_sequencer construction", "[poly_sequencer]") {
  SECTION("Default constructor creates empty poly_sequencer") {
    Poly_sequencer<Poly_test_event> poly;
    REQUIRE(poly.empty());
    REQUIRE(poly.size() == 0);
  }

  SECTION("Construction from sequences with handlers") {
    Poly_event_capture capture;
    std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
        capture.make_handler(0), capture.make_handler(1)};

    std::vector<std::vector<Poly_test_event>> sequences = {
        {Poly_test_event(std::chrono::milliseconds(100), 1),
         Poly_test_event(std::chrono::milliseconds(200), 2)},
        {Poly_test_event(std::chrono::milliseconds(150), 3),
         Poly_test_event(std::chrono::milliseconds(250), 4)}};

    Poly_sequencer<Poly_test_event> poly(handlers, sequences);
    REQUIRE(poly.size() == 2);
  }

  SECTION("Construction throws when handlers and sequences size mismatch") {
    Poly_event_capture capture;
    std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
        capture.make_handler(0)};

    std::vector<std::vector<Poly_test_event>> sequences = {
        {Poly_test_event(std::chrono::milliseconds(100), 1)},
        {Poly_test_event(std::chrono::milliseconds(150), 2)}};

    REQUIRE_THROWS_AS(Poly_sequencer<Poly_test_event>(handlers, sequences),
                      std::invalid_argument);
  }
}

TEST_CASE("Poly_sequencer state queries", "[poly_sequencer]") {
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0), capture.make_handler(1)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(50), 2)},
      {Poly_test_event(std::chrono::milliseconds(50), 3),
       Poly_test_event(std::chrono::milliseconds(50), 4)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

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
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0), capture.make_handler(1)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(50), 2)},
      {Poly_test_event(std::chrono::milliseconds(50), 3),
       Poly_test_event(std::chrono::milliseconds(50), 4)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("start single sequencer") {
    auto start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.start(0, start_time, false);

    REQUIRE(poly.is_scheduling(0));
    REQUIRE_FALSE(poly.is_scheduling(1));
    REQUIRE(poly.any_scheduling());
    REQUIRE_FALSE(poly.all_scheduling());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    poly.pause(0);
    REQUIRE_FALSE(poly.is_scheduling(0));
  }

  SECTION("start_all starts all sequencers") {
    auto start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.start_all(start_time, false);

    REQUIRE(poly.all_scheduling());
    REQUIRE(poly.any_scheduling());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    poly.pause_all();
  }

  SECTION("stop sets position and pauses") {
    auto start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.start(0, start_time, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    auto stop_time = Poly_sequencer<Poly_test_event>::Clock::now();
    poly.stop(0, stop_time, 0);

    REQUIRE_FALSE(poly.is_scheduling(0));
  }

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

  SECTION("stop throws for out of range index") {
    auto stop_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                     std::chrono::milliseconds(100);
    REQUIRE_THROWS_AS(poly.stop(999, stop_time), std::out_of_range);
  }

  SECTION("set_pos throws for out of range index") {
    REQUIRE_THROWS_AS(poly.set_pos(999, 0), std::out_of_range);
  }

  SECTION("set_pos_all for all sequencers") {
    poly.set_pos_all(1);
    REQUIRE(true); // Should not throw
  }

  SECTION("pause_all on stopped sequencers is safe") {
    auto pause_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.pause_all(pause_time);
    REQUIRE_FALSE(poly.any_scheduling());
  }

  SECTION("stop_all on stopped sequencers is safe") {
    auto stop_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                     std::chrono::milliseconds(50);
    poly.stop_all(stop_time, 0);
    REQUIRE_FALSE(poly.any_scheduling());
  }
}

TEST_CASE("Poly_sequencer handler management", "[poly_sequencer]") {
  Poly_event_capture capture1, capture2;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture1.make_handler(0)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("set_handler changes handler") {
    auto start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                      std::chrono::milliseconds(50);
    poly.start(0, start_time, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    poly.pause(0);

    REQUIRE(capture1.total_events() == 1);
    REQUIRE(capture2.total_events() == 0);

    // Change handler and restart
    poly.set_handler(0, capture2.make_handler(0));
    poly.set_pos(0, 0);

    start_time = Poly_sequencer<Poly_test_event>::Clock::now() +
                 std::chrono::milliseconds(50);
    poly.start(0, start_time, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    poly.pause(0);

    REQUIRE(capture1.total_events() == 1); // No new events
    REQUIRE(capture2.total_events() == 1); // New handler received event
  }

  SECTION("set_handler throws for out of range index") {
    REQUIRE_THROWS_AS(poly.set_handler(999, capture2.make_handler(0)),
                      std::out_of_range);
  }
}

TEST_CASE("Poly_sequencer update operations", "[poly_sequencer]") {
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(50), 2),
       Poly_test_event(std::chrono::milliseconds(50), 3)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("update with event object") {
    Poly_test_event new_event(std::chrono::milliseconds(100), 99);
    poly.update(0, 1, new_event);

    auto data = poly[0].snapshot();
    REQUIRE(data[1].id == 99);
    REQUIRE(data[1].duration == std::chrono::milliseconds(100));
  }

  SECTION("update with variadic args") {
    poly.update(0, 1, std::chrono::milliseconds(100), 77);

    auto data = poly[0].snapshot();
    REQUIRE(data[1].id == 77);
    REQUIRE(data[1].duration == std::chrono::milliseconds(100));
  }

  SECTION("update throws for out of range sequencer index") {
    Poly_test_event new_event(std::chrono::milliseconds(100), 99);
    REQUIRE_THROWS_AS(poly.update(999, 0, new_event), std::out_of_range);
  }
}

TEST_CASE("Poly_sequencer duration operations", "[poly_sequencer]") {
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0), capture.make_handler(1)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(100), 2)},
      {Poly_test_event(std::chrono::milliseconds(75), 3),
       Poly_test_event(std::chrono::milliseconds(125), 4)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("adjust_durations on single sequencer") {
    poly.adjust_durations(0, std::chrono::milliseconds(25));

    auto data = poly[0].snapshot();
    REQUIRE(data[0].duration == std::chrono::milliseconds(75));
    REQUIRE(data[1].duration == std::chrono::milliseconds(125));

    // Second sequencer unchanged
    auto data2 = poly[1].snapshot();
    REQUIRE(data2[0].duration == std::chrono::milliseconds(75));
    REQUIRE(data2[1].duration == std::chrono::milliseconds(125));
  }

  SECTION("adjust_durations_all affects all sequencers") {
    poly.adjust_durations_all(std::chrono::milliseconds(25));

    auto data0 = poly[0].snapshot();
    REQUIRE(data0[0].duration == std::chrono::milliseconds(75));
    REQUIRE(data0[1].duration == std::chrono::milliseconds(125));

    auto data1 = poly[1].snapshot();
    REQUIRE(data1[0].duration == std::chrono::milliseconds(100));
    REQUIRE(data1[1].duration == std::chrono::milliseconds(150));
  }

  SECTION("multiply_durations on single sequencer") {
    poly.multiply_durations(0, 2.0);

    auto data = poly[0].snapshot();
    REQUIRE(data[0].duration == std::chrono::milliseconds(100));
    REQUIRE(data[1].duration == std::chrono::milliseconds(200));

    // Second sequencer unchanged
    auto data2 = poly[1].snapshot();
    REQUIRE(data2[0].duration == std::chrono::milliseconds(75));
  }

  SECTION("multiply_durations_all affects all sequencers") {
    poly.multiply_durations_all(0.5);

    auto data0 = poly[0].snapshot();
    REQUIRE(data0[0].duration == std::chrono::milliseconds(25));
    REQUIRE(data0[1].duration == std::chrono::milliseconds(50));

    auto data1 = poly[1].snapshot();
    REQUIRE(data1[0].duration == std::chrono::nanoseconds(37500000)); // 37.5ms
    REQUIRE(data1[1].duration == std::chrono::nanoseconds(62500000)); // 62.5ms
  }

  SECTION("adjust_durations throws for out of range index") {
    REQUIRE_THROWS_AS(poly.adjust_durations(999, std::chrono::milliseconds(25)),
                      std::out_of_range);
  }

  SECTION("multiply_durations throws for out of range index") {
    REQUIRE_THROWS_AS(poly.multiply_durations(999, 2.0), std::out_of_range);
  }
}

TEST_CASE("Poly_sequencer for_each operations", "[poly_sequencer]") {
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0), capture.make_handler(1)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(50), 2)},
      {Poly_test_event(std::chrono::milliseconds(50), 3),
       Poly_test_event(std::chrono::milliseconds(50), 4)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("for_each on single sequencer") {
    poly.for_each(0, [](Poly_test_event& evt) { evt.id += 10; });

    auto data0 = poly[0].snapshot();
    REQUIRE(data0[0].id == 11);
    REQUIRE(data0[1].id == 12);

    // Second sequencer unchanged
    auto data1 = poly[1].snapshot();
    REQUIRE(data1[0].id == 3);
    REQUIRE(data1[1].id == 4);
  }

  SECTION("for_each_all affects all sequencers") {
    poly.for_each_all([](Poly_test_event& evt) { evt.id += 100; });

    auto data0 = poly[0].snapshot();
    REQUIRE(data0[0].id == 101);
    REQUIRE(data0[1].id == 102);

    auto data1 = poly[1].snapshot();
    REQUIRE(data1[0].id == 103);
    REQUIRE(data1[1].id == 104);
  }

  SECTION("for_each throws for out of range index") {
    REQUIRE_THROWS_AS(
        poly.for_each(999, [](Poly_test_event& evt) { evt.id += 10; }),
        std::out_of_range);
  }
}

TEST_CASE("Poly_sequencer replace operations", "[poly_sequencer]") {
  Poly_event_capture capture;
  std::vector<Poly_sequencer<Poly_test_event>::Handler> handlers{
      capture.make_handler(0)};

  std::vector<std::vector<Poly_test_event>> sequences = {
      {Poly_test_event(std::chrono::milliseconds(50), 1),
       Poly_test_event(std::chrono::milliseconds(50), 2),
       Poly_test_event(std::chrono::milliseconds(50), 3)}};

  Poly_sequencer<Poly_test_event> poly(handlers, sequences);

  SECTION("replace single event") {
    Poly_test_event new_event(std::chrono::milliseconds(100), 99);
    poly.replace(0, 1, new_event);

    auto data = poly[0].snapshot();
    REQUIRE(data[0].id == 1);
    REQUIRE(data[1].id == 99);
    REQUIRE(data[1].duration == std::chrono::milliseconds(100));
    REQUIRE(data[2].id == 3);
  }

  SECTION("replace multiple events") {
    std::vector<Poly_test_event> replacements{
        Poly_test_event(std::chrono::milliseconds(100), 88),
        Poly_test_event(std::chrono::milliseconds(100), 99)};

    poly.replace(0, 1, replacements);

    auto data = poly[0].snapshot();
    REQUIRE(data[0].id == 1);
    REQUIRE(data[1].id == 88);
    REQUIRE(data[2].id == 99);
  }

  SECTION("replace throws for out of range sequencer index") {
    Poly_test_event new_event(std::chrono::milliseconds(100), 99);
    REQUIRE_THROWS_AS(poly.replace(999, 0, new_event), std::out_of_range);
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

  SECTION("set_pos_all is safe on empty") {
    poly.set_pos_all(0);
    REQUIRE(poly.empty());
  }

  SECTION("adjust_durations_all is safe on empty") {
    poly.adjust_durations_all(std::chrono::milliseconds(25));
    REQUIRE(poly.empty());
  }

  SECTION("multiply_durations_all is safe on empty") {
    poly.multiply_durations_all(2.0);
    REQUIRE(poly.empty());
  }

  SECTION("for_each_all is safe on empty") {
    poly.for_each_all([](Poly_test_event& evt) { evt.id = 0; });
    REQUIRE(poly.empty());
  }
}

} // namespace poly_sequencer_tests
} // namespace tests
} // namespace Micro_composer
