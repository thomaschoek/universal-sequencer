#include "sequencable/mutable_event.h"
#include "sequencer/sequencer_template.h"
#include <algorithm>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::sequencer;
using namespace Micro_composer::sequencable;

namespace Micro_composer {

namespace tests {

namespace sequencer_tests {

#ifndef NDEBUG

long get_timestamp_ms() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

inline void debug_msg(std::string msg, std::ostream& stream = std::cerr) {
#ifndef NDEBUG
  std::osyncstream(stream) << get_timestamp_ms() << " [TEST_SEQUENCER] "
                           << ": " << msg << std::endl
                           << std::flush;
#endif
}

#endif

// Test event type that properly satisfies Sequencable concept
struct Test_event : public Mutable_event {
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;

  int id{0}; // For tracking events

  Test_event() = default;
  explicit Test_event(Duration d, int event_id = 0) {
    duration = d;
    id = event_id;
    enabled = true;
  }

  // Comparison operators (compare event parameters, not scheduled_time)
  bool operator==(const Test_event& other) const {
    return id == other.id && duration == other.duration &&
           scheduled_time == other.scheduled_time;
  }

  void update(Duration d, int event_id = 0) {
    duration = d;
    id = event_id;
  }

  void update(const Test_event& other) {
    duration = other.duration;
    id = other.id;
  }
};

// Verify Test_event satisfies Sequencable concept
static_assert(Mut_seq_event<Test_event>,
              "Test_event does not satisfy Sequencable concept");

// Helper class to capture events from the handler
class Event_capture {
public:
  std::vector<Test_event> events;
  mutable std::mutex mutex;
  std::condition_variable cv;

  void capture(Test_event&& evt) {
    std::lock_guard<std::mutex> lock(mutex);
    events.push_back(std::move(evt));
    cv.notify_one();
  }

  std::function<void(Test_event&&)> make_handler() {
    return [this](Test_event&& evt) { capture(std::move(evt)); };
  }

  size_t wait_for_events(size_t count, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, timeout,
                [this, count]() { return events.size() >= count; });
    return events.size();
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex);
    events.clear();
  }
};

TEST_CASE("Sequencer construction", "[sequencer]") {
  SECTION("Constructor creates empty sequencer") {
    debug_msg("Starting test: Constructor creates empty sequencer");
    Event_capture capture;
    debug_msg("Creating Sequencer\n");
    Sequencer<Test_event> seq(capture.make_handler());
    debug_msg("DEBUG: Calling empty()");
    REQUIRE(seq.empty());
    debug_msg("DEBUG: Calling size()");
    REQUIRE(seq.size() == 0);
    debug_msg("DEBUG: Test complete\n");
  }

  SECTION("Initializer list constructor") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(100)),
                               Test_event(std::chrono::milliseconds(200)),
                               Test_event(std::chrono::milliseconds(150))});
    REQUIRE_FALSE(seq.empty());
    REQUIRE(seq.size() == 3);
  }
}

TEST_CASE("Sequencer data operations", "[sequencer]") {
  Event_capture capture;
  Sequencer<Test_event> seq(capture.make_handler());

  SECTION("push_back adds events") {
    seq.push_back(Test_event(std::chrono::milliseconds(100), 1));
    REQUIRE(seq.size() == 1);
    seq.push_back(Test_event(std::chrono::milliseconds(200), 2));
    REQUIRE(seq.size() == 2);
  }

  SECTION("push_back rejects events with duration < min_duration") {
    REQUIRE_THROWS_AS(seq.push_back(Test_event(std::chrono::milliseconds(5))),
                      std::invalid_argument);
  }

  SECTION("pop_back removes events") {
    seq.push_back(Test_event(std::chrono::milliseconds(100)));
    seq.push_back(Test_event(std::chrono::milliseconds(200)));
    REQUIRE(seq.size() == 2);
    seq.pop_back();
    REQUIRE(seq.size() == 1);
  }

  SECTION("assign replaces events at position") {
    seq.assign(3, Test_event(std::chrono::milliseconds(100)));
    REQUIRE(seq.size() == 3);
    auto data = seq.snapshot();
    REQUIRE(data.size() == 3);
    REQUIRE(data[0].duration == std::chrono::milliseconds(100));
  }

  SECTION("assign with initializer list replaces all events") {
    seq.push_back(Test_event(std::chrono::milliseconds(100)));
    seq.assign({Test_event(std::chrono::milliseconds(50)),
                Test_event(std::chrono::milliseconds(75))});
    REQUIRE(seq.size() == 2);
  }

  SECTION("assign with vector replaces all events") {
    std::vector<Test_event> events = {
        Test_event(std::chrono::milliseconds(100)),
        Test_event(std::chrono::milliseconds(200)),
        Test_event(std::chrono::milliseconds(300))};
    seq.assign(events);
    REQUIRE(seq.size() == 3);
  }

  SECTION("clear removes all events and stops sequencer") {
    seq.push_back(Test_event(std::chrono::milliseconds(100)));
    seq.push_back(Test_event(std::chrono::milliseconds(200)));
    seq.clear();
    REQUIRE(seq.empty());
    REQUIRE(seq.size() == 0);
    REQUIRE_FALSE(seq.is_scheduling());
  }

  SECTION("data returns copy of events") {
    seq.push_back(Test_event(std::chrono::milliseconds(100), 1));
    seq.push_back(Test_event(std::chrono::milliseconds(200), 2));
    auto data = seq.snapshot();
    REQUIRE(data.size() == 2);
    REQUIRE(data[0].duration == std::chrono::milliseconds(100));
    REQUIRE(data[0].id == 1);
    REQUIRE(data[1].duration == std::chrono::milliseconds(200));
    REQUIRE(data[1].id == 2);
  }

  SECTION("insert adds event at specified position") {
    seq.push_back(Test_event(std::chrono::milliseconds(100), 1));
    seq.push_back(Test_event(std::chrono::milliseconds(200), 3));
    seq.insert(1, Test_event(std::chrono::milliseconds(150), 2));
    REQUIRE(seq.size() == 3);
    auto data = seq.snapshot();
    REQUIRE(data[1].id == 2);
    REQUIRE(data[1].duration == std::chrono::milliseconds(150));
  }

  SECTION("erase removes event at specified position") {
    seq.push_back(Test_event(std::chrono::milliseconds(100), 1));
    seq.push_back(Test_event(std::chrono::milliseconds(200), 2));
    seq.push_back(Test_event(std::chrono::milliseconds(300), 3));
    seq.erase(1);
    REQUIRE(seq.size() == 2);
    auto data = seq.snapshot();
    REQUIRE(data[0].id == 1);
    REQUIRE(data[1].id == 3);
  }

  SECTION("erase throws on out of range index") {
    seq.push_back(Test_event(std::chrono::milliseconds(100)));
    REQUIRE_THROWS_AS(seq.erase(5), std::out_of_range);
  }
}

TEST_CASE("Sequencer transport control", "[sequencer]") {
  Event_capture capture;
  Sequencer<Test_event> seq(capture.make_handler(),
                            {Test_event(std::chrono::milliseconds(50), 1),
                             Test_event(std::chrono::milliseconds(50), 2),
                             Test_event(std::chrono::milliseconds(50), 3)});

  SECTION("Sequencer starts in stopped state") {
    REQUIRE_FALSE(seq.is_scheduling());
  }

  //  SECTION("start with past time throws") {
  //    auto past_time =
  //        Sequencer<Test_event>::Clock::now() -
  //        std::chrono::milliseconds(100);
  //    REQUIRE_THROWS_AS(seq.start(past_time), std::invalid_argument);
  //  }

  SECTION("start activates scheduling") {
    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);
    REQUIRE(seq.is_scheduling());
    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(20));
    REQUIRE_FALSE(seq.is_scheduling());
  }

  SECTION("Pausing already stopped sequencer is safe") {
    REQUIRE_FALSE(seq.is_scheduling());
    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(50));
    REQUIRE_FALSE(seq.is_scheduling());
  }

  SECTION("stop sets position and pauses") {
    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);
    REQUIRE(seq.is_scheduling());
    seq.stop(
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(20), 1);
    REQUIRE_FALSE(seq.is_scheduling());
    REQUIRE(seq.get_pos() == 1);
  }

  SECTION("starting already running sequencer is no-op") {
    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);
    REQUIRE(seq.is_scheduling());
    seq.start(start_time, false); // Should be no-op
    REQUIRE(seq.is_scheduling());
    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(20));
  }
}

TEST_CASE("Sequencer event retrieval", "[sequencer]") {
  SECTION("Handler receives scheduled events in once mode") {
    debug_msg("[TEST SECTION] Handler receives scheduled events in once ");
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(10), 1),
                               Test_event(std::chrono::milliseconds(10), 2),
                               Test_event(std::chrono::milliseconds(10), 3)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    debug_msg("[DEBUG] Calling seq.start() at ");
    seq.start(start_time, false);
    debug_msg("Returned from seq.start()");

    // Wait for all events to be delivered
    REQUIRE(capture.wait_for_events(3, std::chrono::milliseconds(300)) == 3);

    std::lock_guard<std::mutex> lock(capture.mutex);
    REQUIRE(capture.events[0].id == 1);
    REQUIRE(capture.events[0].scheduled_time >= start_time);

    REQUIRE(capture.events[1].id == 2);
    REQUIRE(capture.events[1].scheduled_time >=
            capture.events[0].scheduled_time + capture.events[0].duration);

    REQUIRE(capture.events[2].id == 3);
    REQUIRE(capture.events[2].scheduled_time >=
            capture.events[1].scheduled_time + capture.events[1].duration);

    // Wait for sequencer to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    debug_msg(
        "[TEST SECTION] Handler receives scheduled events in once mode DONE");
  }

  SECTION("Handler receives events in repeat mode") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(40), 1),
                               Test_event(std::chrono::milliseconds(40), 2)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, true);

    // Wait for multiple repetitions
    REQUIRE(capture.wait_for_events(5, std::chrono::milliseconds(500)) >= 5);

    std::vector<int> received_ids;
    {
      std::lock_guard<std::mutex> lock(capture.mutex);
      for (const auto& evt : capture.events) {
        received_ids.push_back(evt.id);
      }
    }

    // Should have looped at least once
    REQUIRE(std::count(received_ids.begin(), received_ids.end(), 1) >= 2);
    REQUIRE(std::count(received_ids.begin(), received_ids.end(), 2) >= 2);

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }

  SECTION("t_next returns next scheduled time") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);

    std::this_thread::sleep_until(start_time + std::chrono::milliseconds(50));

    // t_next should be updated as events are scheduled
    auto next_time = seq.t_next();
    REQUIRE(next_time >= start_time);

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }
}

TEST_CASE("Sequencer position control", "[sequencer]") {
  Event_capture capture;
  Sequencer<Test_event> seq(capture.make_handler(),
                            {Test_event(std::chrono::milliseconds(50), 1),
                             Test_event(std::chrono::milliseconds(50), 2),
                             Test_event(std::chrono::milliseconds(50), 3)});

  SECTION("get_pos returns current position") { REQUIRE(seq.get_pos() == 0); }

  SECTION("set_pos changes position") {
    seq.set_pos(1);
    REQUIRE(seq.get_pos() == 1);
    seq.set_pos(2);
    REQUIRE(seq.get_pos() == 2);
  }

  SECTION("set_pos throws on out of range") {
    REQUIRE_THROWS_AS(seq.set_pos(10), std::out_of_range);
  }

  SECTION("set_pos affects which event is retrieved next") {
    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.set_pos(1);
    seq.start(start_time, false);

    // Wait for first event
    REQUIRE(capture.wait_for_events(1, std::chrono::milliseconds(200)) >= 1);

    {
      std::lock_guard<std::mutex> lock(capture.mutex);
      REQUIRE(capture.events[0].id ==
              2); // Should start from position 1 (second event)
    }

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }
}

TEST_CASE("Sequencer thread safety", "[sequencer][concurrency]") {
  SECTION("Data operations are safe while not scheduling") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler());

    std::thread writer1([&]() {
      for (int i = 0; i < 10; ++i) {
        seq.push_back(Test_event(std::chrono::milliseconds(50), i));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });

    std::thread writer2([&]() {
      for (int i = 10; i < 20; ++i) {
        seq.push_back(Test_event(std::chrono::milliseconds(50), i));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });

    writer1.join();
    writer2.join();

    REQUIRE(seq.size() == 20);
  }

  SECTION("Concurrent reads are safe") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    std::atomic<bool> done{false};
    std::vector<std::thread> readers;

    for (int i = 0; i < 5; ++i) {
      readers.emplace_back([&]() {
        while (!done.load()) {
          auto data = seq.snapshot();
          REQUIRE(data.size() == 3);
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    done.store(true);

    for (auto& t : readers) {
      t.join();
    }
  }

  SECTION("Position can be safely queried concurrently") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2)});

    std::atomic<bool> done{false};
    std::vector<std::thread> readers;

    for (int i = 0; i < 3; ++i) {
      readers.emplace_back([&]() {
        while (!done.load()) {
          auto pos = seq.get_pos();
          REQUIRE(pos < 2);
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    done.store(true);

    for (auto& t : readers) {
      t.join();
    }
  }
}

TEST_CASE("Sequencer on-the-fly modifications", "[sequencer][concurrency]") {
  SECTION("Can modify events while scheduling") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler());
    // Create events with longer durations to have time for modifications
    for (int i = 0; i < 10; ++i) {
      seq.push_back(Test_event(std::chrono::milliseconds(50), i));
    }

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, true);

    // Wait for a couple of events
    capture.wait_for_events(2, std::chrono::milliseconds(200));

    // Modify an event that hasn't been scheduled yet
    // Insert waits until it's safe to modify
    seq.push_back(Test_event(std::chrono::milliseconds(50), 100));

    REQUIRE(seq.size() == 11);

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }

  SECTION("Can erase events while scheduling") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler());
    for (int i = 0; i < 10; ++i) {
      seq.push_back(Test_event(std::chrono::milliseconds(40), i));
    }

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, true);

    // Wait for first event
    capture.wait_for_events(1, std::chrono::milliseconds(150));

    // Erase an event that's far ahead
    // This should be safe since we're not at that position
    seq.erase(8);

    REQUIRE(seq.size() == 9);

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }

  SECTION("Can insert events while scheduling") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler());
    debug_msg("[DEBUG]: Pushing initial events\n");
    for (int i = 0; i < 5; ++i) {
      seq.push_back(Test_event(std::chrono::milliseconds(40), i));
    }

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    debug_msg("[DEBUG]: Starting sequencer\n");
    seq.start(start_time, true);

    // Wait for first event
    debug_msg("[DEBUG]: Waiting for events\n");
    capture.wait_for_events(1, std::chrono::milliseconds(150));

    // Insert at end (should be safe)
    debug_msg("[DEBUG]: Pushing event back while running\n");
    seq.push_back(Test_event(std::chrono::milliseconds(40), 99));

    REQUIRE(seq.size() == 6);

    debug_msg("[DEBUG]: Pausing sequencer\n");
    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
    debug_msg("[DEBUG]: SECTION Can insert events while scheduling DONE\n");
  }
}

TEST_CASE("Sequencer move constructor", "[sequencer]") {
  SECTION("Move constructor transfers state correctly") {
    Event_capture capture;
    Sequencer<Test_event> seq1(capture.make_handler(),
                               {Test_event(std::chrono::milliseconds(50), 1),
                                Test_event(std::chrono::milliseconds(50), 2)});

    seq1.set_pos(1);

    Sequencer<Test_event> seq2(std::move(seq1));

    REQUIRE(seq2.size() == 2);
    auto data = seq2.snapshot();
    REQUIRE(data[0].id == 1);
    REQUIRE(data[1].id == 2);
  }

  SECTION("Move constructor stops source sequencer if running") {
    debug_msg("[DEBUG]: SECTION Move constructor stops source sequencer if "
              "running START\n");
    Event_capture capture;
    Sequencer<Test_event> seq1(capture.make_handler(),
                               {Test_event(std::chrono::milliseconds(50), 1),
                                Test_event(std::chrono::milliseconds(50), 2)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);

    debug_msg("[DEBUG]: Starting sequencer\n");
    seq1.start(start_time, false);
    REQUIRE(seq1.is_scheduling());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    debug_msg("[DEBUG]: Moving sequencer while it is running\n");
    Sequencer<Test_event> seq2(std::move(seq1));

    REQUIRE_FALSE(seq2.is_scheduling());
  }
}

TEST_CASE("Sequencer timing accuracy", "[sequencer][timing]") {
  SECTION("Events are scheduled with reasonable accuracy") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(100);
    seq.start(start_time, false);

    // Wait for events to be delivered
    REQUIRE(capture.wait_for_events(2, std::chrono::milliseconds(300)) >= 2);

    std::lock_guard<std::mutex> lock(capture.mutex);
    auto evt1 = capture.events[0];
    auto evt2 = capture.events[1];

    // Check that events were scheduled reasonably close to expected time
    // Note: we check against start_time, not current time, since the handler
    // is called after the event is scheduled
    REQUIRE(evt1.scheduled_time >= start_time);
    REQUIRE(evt2.scheduled_time >= evt1.scheduled_time + evt1.duration);

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }

  SECTION("Event durations are respected") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(100), 1),
                               Test_event(std::chrono::milliseconds(50), 2)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);

    // Wait for events
    REQUIRE(capture.wait_for_events(2, std::chrono::milliseconds(300)) >= 2);

    std::lock_guard<std::mutex> lock(capture.mutex);
    auto evt1 = capture.events[0];
    auto evt2 = capture.events[1];

    // Second event should be scheduled duration of first event after first
    auto expected_gap = std::chrono::milliseconds(100);
    auto actual_gap = evt2.scheduled_time - evt1.scheduled_time;
    auto gap_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                        actual_gap - expected_gap)
                        .count();

    REQUIRE(std::abs(gap_diff) < 20); // Within 20ms tolerance

    seq.pause(Sequencer<Test_event>::Clock::now() +
              std::chrono::milliseconds(10));
  }
}

TEST_CASE("Sequencer set_handler", "[sequencer]") {
  SECTION("set_handler changes event handler") {
    int handler1_calls = 0;
    int handler2_calls = 0;

    auto handler1 = [&handler1_calls](Test_event&&) { handler1_calls++; };
    auto handler2 = [&handler2_calls](Test_event&&) { handler2_calls++; };

    Sequencer<Test_event> seq(handler1,
                              {Test_event(std::chrono::milliseconds(50), 1)});

    auto start_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(start_time, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    seq.pause();

    REQUIRE(handler1_calls == 1);
    REQUIRE(handler2_calls == 0);

    // Change handler and restart
    seq.set_handler(handler2);
    seq.set_pos(0);
    auto restart_time =
        Sequencer<Test_event>::Clock::now() + std::chrono::milliseconds(50);
    seq.start(restart_time, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    seq.pause();

    REQUIRE(handler1_calls == 1); // No new calls to handler1
    REQUIRE(handler2_calls == 1); // New handler was called
  }
}

TEST_CASE("Sequencer update operations", "[sequencer]") {
  SECTION("update with event object updates event at position") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    Test_event new_event(std::chrono::milliseconds(100), 99);
    seq.update(1, new_event);

    auto events = seq.snapshot();
    REQUIRE(events.size() == 3);
    REQUIRE(events[0].id == 1);
    REQUIRE(events[1].id == 99);
    REQUIRE(events[1].duration == std::chrono::milliseconds(100));
    REQUIRE(events[2].id == 3);
  }

  SECTION("update with variadic args updates event at position") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    seq.update(1, std::chrono::milliseconds(100), 77);

    auto events = seq.snapshot();
    REQUIRE(events.size() == 3);
    REQUIRE(events[0].id == 1);
    REQUIRE(events[1].id == 77);
    REQUIRE(events[1].duration == std::chrono::milliseconds(100));
    REQUIRE(events[2].id == 3);
  }

  SECTION("update throws on out of range index") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1)});

    Test_event new_event(std::chrono::milliseconds(100), 99);
    REQUIRE_THROWS_AS(seq.update(10, new_event), std::out_of_range);
  }
}

TEST_CASE("Sequencer duration operations", "[sequencer]") {
  SECTION("adjust_durations adds delta to all durations") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(100), 2),
                               Test_event(std::chrono::milliseconds(150), 3)});

    auto control_event = Test_event{std::chrono::milliseconds(50), 3};
    control_event.set_duration(std::chrono::milliseconds(75));
    REQUIRE(control_event.duration == std::chrono::milliseconds(75));

    seq.adjust_durations(std::chrono::milliseconds(25));

    auto events = seq.snapshot();
    REQUIRE(events[0].duration == std::chrono::milliseconds(75));
    REQUIRE(events[1].duration == std::chrono::milliseconds(125));
    REQUIRE(events[2].duration == std::chrono::milliseconds(175));
  }

  SECTION("adjust_durations can subtract from durations") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(100), 2)});

    seq.adjust_durations(std::chrono::milliseconds(-20));

    auto events = seq.snapshot();
    REQUIRE(events[0].duration == std::chrono::milliseconds(30));
    REQUIRE(events[1].duration == std::chrono::milliseconds(80));
  }

  SECTION("multiply_durations scales all durations") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(100), 1),
                               Test_event(std::chrono::milliseconds(200), 2),
                               Test_event(std::chrono::milliseconds(300), 3)});

    seq.multiply_durations(2.0);

    auto events = seq.snapshot();
    REQUIRE(events[0].duration == std::chrono::milliseconds(200));
    REQUIRE(events[1].duration == std::chrono::milliseconds(400));
    REQUIRE(events[2].duration == std::chrono::milliseconds(600));
  }

  SECTION("multiply_durations works with fractional factors") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(100), 1),
                               Test_event(std::chrono::milliseconds(200), 2)});

    seq.multiply_durations(0.5);

    auto events = seq.snapshot();
    REQUIRE(events[0].duration == std::chrono::milliseconds(50));
    REQUIRE(events[1].duration == std::chrono::milliseconds(100));
  }

  SECTION("multiply_durations throws on negative factor") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(100), 1)});

    REQUIRE_THROWS_AS(seq.multiply_durations(-1.0), std::invalid_argument);
  }
}

TEST_CASE("Sequencer for_each operation", "[sequencer]") {
  SECTION("for_each applies function to all events") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    seq.mutate([](Test_event&& evt) {
      evt.id += 10;
      return std::move(evt);
    });

    auto events = seq.snapshot();
    REQUIRE(events[0].id == 11);
    REQUIRE(events[1].id == 12);
    REQUIRE(events[2].id == 13);
  }

  SECTION("for_each can modify durations") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(100), 2)});

    seq.mutate([](Test_event&& evt) {
      evt.duration = std::chrono::milliseconds(200);
      return std::move(evt);
    });

    auto events = seq.snapshot();
    REQUIRE(events[0].duration == std::chrono::milliseconds(200));
    REQUIRE(events[1].duration == std::chrono::milliseconds(200));
  }
}

TEST_CASE("Sequencer replace operations", "[sequencer]") {
  SECTION("replace single event at position") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3)});

    Test_event replacement(std::chrono::milliseconds(100), 99);
    seq.replace(1, replacement);

    auto events = seq.snapshot();
    REQUIRE(events.size() == 3);
    REQUIRE(events[0].id == 1);
    REQUIRE(events[1].id == 99);
    REQUIRE(events[1].duration == std::chrono::milliseconds(100));
    REQUIRE(events[2].id == 3);
  }

  SECTION("replace throws on out of range index") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1)});

    Test_event replacement(std::chrono::milliseconds(100), 99);
    REQUIRE_THROWS_AS(seq.replace(10, replacement), std::out_of_range);
  }

  SECTION("replace multiple events from position") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2),
                               Test_event(std::chrono::milliseconds(50), 3),
                               Test_event(std::chrono::milliseconds(50), 4)});

    std::vector<Test_event> replacements{
        Test_event(std::chrono::milliseconds(100), 88),
        Test_event(std::chrono::milliseconds(100), 99)};

    seq.replace(1, replacements);

    auto events = seq.snapshot();
    REQUIRE(events.size() == 4);
    REQUIRE(events[0].id == 1);
    REQUIRE(events[1].id == 88);
    REQUIRE(events[2].id == 99);
    REQUIRE(events[3].id == 4);
  }

  SECTION("replace multiple throws if out of range") {
    Event_capture capture;
    Sequencer<Test_event> seq(capture.make_handler(),
                              {Test_event(std::chrono::milliseconds(50), 1),
                               Test_event(std::chrono::milliseconds(50), 2)});

    std::vector<Test_event> replacements{
        Test_event(std::chrono::milliseconds(100), 88),
        Test_event(std::chrono::milliseconds(100), 99),
        Test_event(std::chrono::milliseconds(100), 77)};

    REQUIRE_THROWS_AS(seq.replace(1, replacements), std::out_of_range);
  }
}

} // namespace sequencer_tests

} // namespace tests

} // namespace Micro_composer
