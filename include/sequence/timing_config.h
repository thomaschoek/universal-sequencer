#ifndef SEQUENCER_TIMING_CONFIG_H
#define SEQUENCER_TIMING_CONFIG_H

#ifndef NDEBUG
#include <iostream>
#endif

#include <chrono>
#include <mutex>
#include <thread>

namespace MicroComposer {
namespace timing_capabilities {

constexpr const uint test_iterations{1000};
constexpr const std::chrono::microseconds test_sleep_time{1000}; // 1ms

// Timing configuration based on system capabilities
struct TimingCapabilities {

  std::chrono::nanoseconds precision;
  std::chrono::milliseconds min_interval;

  // Detect system timing capabilities
  TimingCapabilities() {

    // Measure actual sleep precision by testing short sleeps
    std::chrono::nanoseconds total_error{0};

    for (uint i = 0; i < test_iterations; ++i) {
      auto start_time = std::chrono::steady_clock::now();
      auto target_wake_time = start_time + test_sleep_time;
      std::this_thread::sleep_until(target_wake_time);
      auto end = std::chrono::steady_clock::now();
      auto actual_sleep_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                               start_time);
      total_error += std::chrono::abs(actual_sleep_time - test_sleep_time);
    }

    std::chrono::nanoseconds precision_ = total_error / test_iterations;

    std::chrono::milliseconds min_interval_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(precision * 10);

#ifndef NDEBUG
    std::cout << "[DEBUG] Detected timing capabilities:" << std::endl;
    std::cout << "  Sleep precision: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     precision_)
                     .count()
              << "μs" << std::endl;
    std::cout << "  Min interval: " << min_interval_.count() << "ms"
              << std::endl;
#endif
    precision = precision_;
    min_interval = min_interval_;
  }

  static const TimingCapabilities &get_instance() {
    static std::once_flag initialized;
    static TimingCapabilities instance;
    std::call_once(initialized, []() {
      // Constructor already does the work
    });
    return instance;
  }
};

} // namespace timing_capabilities
} // namespace MicroComposer

#endif