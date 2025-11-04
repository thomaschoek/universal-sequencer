#include <catch2/catch_test_macros.hpp>
#include "sequencer/sequencer_template.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

using namespace Micro_composer;
using namespace Micro_composer::sequencer;

// Test event for race condition testing
struct Race_test_event {
  bool enabled{true};
  sequencable::Duration duration{std::chrono::milliseconds(100)};
  sequencable::Time_point scheduled_time{};
  int value{0};

  void set_duration(sequencable::Duration d) { duration = d; }
  void set_scheduled_time(sequencable::Time_point t) { scheduled_time = t; }
  void update(const Race_test_event& other) {
    enabled = other.enabled;
    duration = other.duration;
    value = other.value;
  }
};

// Helper class to capture events from the handler
class Event_capture {
public:
  std::vector<Race_test_event> events;
  mutable std::mutex mutex;
  std::condition_variable cv;

  void capture(Race_test_event&& evt) {
    std::lock_guard<std::mutex> lock(mutex);
    events.push_back(std::move(evt));
    cv.notify_one();
  }

  std::function<void(Race_test_event&&)> make_handler() {
    return [this](Race_test_event&& evt) { capture(std::move(evt)); };
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

TEST_CASE("Sequencer::insert() - basic functionality", "[sequencer][insert]") {
  Event_capture capture;
  Sequencer<Race_test_event> seq(capture.make_handler());

  // Add initial events
  seq.push_back(Race_test_event{true, std::chrono::milliseconds(100), {}, 1});
  seq.push_back(Race_test_event{true, std::chrono::milliseconds(100), {}, 2});
  seq.push_back(Race_test_event{true, std::chrono::milliseconds(100), {}, 3});

  REQUIRE(seq.size() == 3);

  SECTION("Insert at beginning increases size") {
    Race_test_event new_event{true, std::chrono::milliseconds(100), {}, 0};
    seq.insert(0, new_event);
    REQUIRE(seq.size() == 4);
  }

  SECTION("Insert in middle increases size") {
    Race_test_event new_event{true, std::chrono::milliseconds(100), {}, 99};
    seq.insert(1, new_event);
    REQUIRE(seq.size() == 4);
  }

  SECTION("Insert near end increases size") {
    Race_test_event new_event{true, std::chrono::milliseconds(100), {}, 4};
    seq.insert(2, new_event);  // Insert at position 2 (before last element)
    REQUIRE(seq.size() == 4);
  }
}

TEST_CASE("Sequencer::insert() - playhead position tracking", "[sequencer][insert][!mayfail]") {
  using Clock = std::chrono::steady_clock;
  Event_capture capture;
  Sequencer<Race_test_event> seq(capture.make_handler());

  // Create sequencer with 5 events
  for (int i = 0; i < 5; ++i) {
    seq.push_back(Race_test_event{true, std::chrono::milliseconds(50), {}, i});
  }

  REQUIRE(seq.size() == 5);

  SECTION("Insert before current position - playhead should increment") {
    // Start scheduling
    auto start_time = Clock::now();
    seq.start(start_time, true);

    // Wait until playhead is at position 2
    while (seq.get_pos() < 2) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto current_before = seq.get_pos();
    REQUIRE(current_before >= 2);

    // Insert event at position 0 (before current)
    Race_test_event new_event{true, std::chrono::milliseconds(50), {}, 99};
    seq.insert(0, new_event);

    // Playhead should be incremented by 1 (because we inserted before it)
    auto current_after = seq.get_pos();

    // TOCTOU BUG: This may FAIL because current_before could be stale
    // If the scheduler advanced between load (line 211) and check (line 223),
    // the playhead increment might be incorrect
    REQUIRE(current_after == current_before + 1);

    seq.pause(Clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  SECTION("Insert after current position - playhead should not change") {
    // Start scheduling
    auto start_time = Clock::now();
    seq.start(start_time, true);

    // Wait until playhead is at position 1
    while (seq.get_pos() < 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto current_before = seq.get_pos();
    REQUIRE(current_before >= 1);
    REQUIRE(current_before < 5);

    // Insert event at position 4 (after current)
    Race_test_event new_event{true, std::chrono::milliseconds(50), {}, 99};
    seq.insert(4, new_event);

    // Playhead should NOT change (we inserted after it)
    auto current_after = seq.get_pos();
    REQUIRE(current_after == current_before);

    seq.pause(Clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_CASE("Sequencer::insert() - TOCTOU race condition", "[sequencer][insert][race][!mayfail]") {
  using Clock = std::chrono::steady_clock;

  SECTION("Race between insert and scheduler advancing playhead") {
    Event_capture capture;
    Sequencer<Race_test_event> seq(capture.make_handler());

    // Create sequencer with fast events to increase race window
    for (int i = 0; i < 10; ++i) {
      seq.push_back(Race_test_event{true, std::chrono::milliseconds(20), {}, i});
    }

    // Start scheduling
    auto start_time = Clock::now();
    seq.start(start_time, true);

    // Wait until playhead advances
    while (seq.get_pos() < 3) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::atomic<bool> insert_done{false};
    std::atomic<size_t> position_at_insert{0};
    std::atomic<size_t> position_after_insert{0};

    // Thread that inserts event at position 1 (before current playhead)
    std::thread inserter([&]() {
      position_at_insert.store(seq.get_pos());

      Race_test_event new_event{true, std::chrono::milliseconds(20), {}, 99};
      seq.insert(1, new_event);

      position_after_insert.store(seq.get_pos());
      insert_done.store(true);
    });

    inserter.join();

    // Check results
    auto pos_at = position_at_insert.load();
    auto pos_after = position_after_insert.load();

    INFO("Position at insert: " << pos_at);
    INFO("Position after insert: " << pos_after);

    // Expected behavior: If current was > 1 at insert time,
    // it should be incremented by 1 to account for the inserted event
    if (pos_at > 1) {
      // TOCTOU bug: This may FAIL because the check at line 223 uses stale value
      // The playhead might not be incremented correctly if scheduler advanced
      // between the load (line 211) and the check (line 223)
      REQUIRE(pos_after == pos_at + 1);
    }

    seq.pause(Clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  SECTION("Multiple concurrent inserts") {
    Event_capture capture;
    Sequencer<Race_test_event> seq(capture.make_handler());

    // Create initial sequence
    for (int i = 0; i < 20; ++i) {
      seq.push_back(Race_test_event{true, std::chrono::milliseconds(30), {}, i});
    }

    // Start scheduling
    auto start_time = Clock::now();
    seq.start(start_time, true);

    // Wait for playhead to advance
    while (seq.get_pos() < 5) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::atomic<int> successful_inserts{0};
    std::vector<std::thread> threads;

    // Spawn multiple threads that insert at different positions
    for (int i = 0; i < 5; ++i) {
      threads.emplace_back([&, i]() {
        try {
          Race_test_event new_event{true, std::chrono::milliseconds(30), {}, 100 + i};
          seq.insert(i * 2, new_event);
          successful_inserts.fetch_add(1);
        } catch (...) {
          // Insert may fail if position is out of range
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    INFO("Successful inserts: " << successful_inserts.load());
    INFO("Final sequence size: " << seq.size());

    // At least some inserts should succeed
    REQUIRE(successful_inserts.load() > 0);

    seq.pause(Clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_CASE("Sequencer::insert() - edge cases", "[sequencer][insert]") {
  Event_capture capture;
  Sequencer<Race_test_event> seq(capture.make_handler());

  // Add events
  for (int i = 0; i < 3; ++i) {
    seq.push_back(Race_test_event{true, std::chrono::milliseconds(100), {}, i});
  }

  SECTION("Insert at current playhead position") {
    using Clock = std::chrono::steady_clock;

    // Start scheduling
    auto start_time = Clock::now() + std::chrono::milliseconds(100);
    seq.start(start_time, true);

    // Insert at position 0 (current position)
    Race_test_event new_event{true, std::chrono::milliseconds(100), {}, 99};

    // This should wait until current advances past position 0
    seq.insert(0, new_event);

    REQUIRE(seq.size() == 4);

    seq.pause(Clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  SECTION("Insert with invalid position") {
    Race_test_event new_event{true, std::chrono::milliseconds(100), {}, 99};

    // Should throw for out-of-range position
    REQUIRE_THROWS(seq.insert(100, new_event));
  }
}
