#include "sequencer/atomic_sequencer.tpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

using namespace Micro_composer;

// Simple test event that satisfies sequencable concept
struct TestEvent {
  int id;
  std::chrono::duration<double> offset{0.1};
  std::chrono::duration<double> duration{0.05};
};

// Test handler that tracks execution order and timing
class AsyncTestHandler {
public:
  AsyncTestHandler() = default;

  // Make it movable but not copyable
  AsyncTestHandler(const AsyncTestHandler&) = delete;
  AsyncTestHandler& operator=(const AsyncTestHandler&) = delete;
  AsyncTestHandler(AsyncTestHandler&&) = default;
  AsyncTestHandler& operator=(AsyncTestHandler&&) = default;

  void operator()(TestEvent&& event) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work

    auto now = std::chrono::steady_clock::now();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      handled_events_.emplace_back(event.id, now);
      ++call_count_;
    }
  }

  std::vector<std::pair<int, std::chrono::steady_clock::time_point>>
  get_handled_events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handled_events_;
  }

  size_t get_call_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return call_count_;
  }

private:
  mutable std::mutex mutex_;
  std::vector<std::pair<int, std::chrono::steady_clock::time_point>>
      handled_events_;
  size_t call_count_{0};
};

TEST_CASE("Async AtomicSequencer timing precision", "[async_sequencer]") {
  sequence::Atomic_step_sequence<TestEvent> sequence;

  // Add test events with specific IDs
  sequence.push_back({1});
  sequence.push_back({2});
  sequence.push_back({3});

  // Use a shared handler state for testing
  auto handler_state = std::make_shared<
      std::vector<std::pair<int, std::chrono::steady_clock::time_point>>>();
  auto handler_mutex = std::make_shared<std::mutex>();

  auto handler = [handler_state, handler_mutex](TestEvent&& event) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work

    auto now = std::chrono::steady_clock::now();
    {
      std::lock_guard<std::mutex> lock(*handler_mutex);
      handler_state->emplace_back(event.id, now);
    }
  };

  sequencer::Atomic_sequencer<TestEvent> seq(handler, sequence);

  auto start_time = std::chrono::steady_clock::now();
  seq.start();

  // Let it run for enough time to process several events
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  seq.stop();

  std::vector<std::pair<int, std::chrono::steady_clock::time_point>>
      handled_events;
  {
    std::lock_guard<std::mutex> lock(*handler_mutex);
    handled_events = *handler_state;
  }

  SECTION("Events are processed in order") {
    REQUIRE(handled_events.size() >= 3);
    REQUIRE(handled_events[0].first == 1);
    REQUIRE(handled_events[1].first == 2);
    REQUIRE(handled_events[2].first == 3);
  }

  SECTION("Events are processed with correct timing") {
    // Each event has 0.1s offset, so spacing should be approximately 0.1s
    if (handled_events.size() >= 2) {
      auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
          handled_events[1].second - handled_events[0].second);

      // Allow some tolerance for timing precision
      REQUIRE(time_diff.count() >= 90);  // At least 90ms
      REQUIRE(time_diff.count() <= 150); // At most 150ms
    }
  }
}

TEST_CASE("Async processing doesn't block sequencer timing",
          "[async_sequencer]") {
  sequence::Atomic_step_sequence<TestEvent> sequence;

  // Add events with short intervals
  for (int i = 0; i < 10; ++i) {
    TestEvent event;
    event.id = i;
    event.offset = std::chrono::duration<double>(0.02);   // 20ms intervals
    event.duration = std::chrono::duration<double>(0.01); // 10ms duration
    sequence.push_back(event);
  }

  // Use a shared handler state for testing
  auto handler_state = std::make_shared<std::vector<int>>();
  auto handler_mutex = std::make_shared<std::mutex>();

  auto handler = [handler_state, handler_mutex](TestEvent&& event) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)); // Slow handler to test non-blocking

    {
      std::lock_guard<std::mutex> lock(*handler_mutex);
      handler_state->push_back(event.id);
    }
  };

  sequencer::Atomic_sequencer<TestEvent> seq(handler, sequence);

  auto start_time = std::chrono::steady_clock::now();
  seq.start();

  // Run for enough time to trigger many events
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  seq.stop();

  std::vector<int> handled_events;
  {
    std::lock_guard<std::mutex> lock(*handler_mutex);
    handled_events = *handler_state;
  }

  // With async processing, we should be able to process many more events
  // than if the sequencer was blocked by handler execution time
  REQUIRE(handled_events.size() >=
          6); // Should process multiple events despite slow handler
}