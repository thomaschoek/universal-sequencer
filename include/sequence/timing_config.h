#ifndef SEQUENCER_TIMING_CONFIG_H
#define SEQUENCER_TIMING_CONFIG_H

#ifndef NDEBUG
#include <iostream>
#endif

#include <chrono>
#include <execution>
#include <mutex>
#include <numeric>
#include <thread>
#include <valarray>
#include <vector>

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

    typedef std::chrono::nanoseconds::rep nanoseconds_cnt_t;

    // Measure actual sleep precision by testing short sleeps
    std::valarray<nanoseconds_cnt_t> sleep_times(test_iterations);

    for (auto &sleep_time : sleep_times) {
      const auto start_time = std::chrono::steady_clock::now();
      const auto target_wake_time = start_time + test_sleep_time;
      std::this_thread::sleep_until(target_wake_time);
      const auto end_time = std::chrono::steady_clock::now();
      sleep_time = (end_time - start_time).count();
    }

    const nanoseconds_cnt_t total_sleep_time = sleep_times.sum();

    std::cout << "[DEBUG] Total sleep time over " << test_iterations
              << " iterations: " << total_sleep_time << "ns" << std::endl;

    const nanoseconds_cnt_t mean_sleep_time =
        total_sleep_time / sleep_times.size();

    std::cout << "[DEBUG] Mean sleep time over " << test_iterations
              << " iterations: " << mean_sleep_time << "ns" << std::endl;

    // Parallel standard deviation calculation
    const nanoseconds_cnt_t variance =
        std::transform_reduce(std::execution::par_unseq,
                              std::begin(sleep_times), std::end(sleep_times),
                              0.0, std::plus{},
                              [mean_sleep_time](nanoseconds_cnt_t sleep_time) {
                                return pow(sleep_time - mean_sleep_time, 2);
                              }) /
        sleep_times.size();

    std::cout << "[DEBUG] Sleep time variance over " << test_iterations
              << " iterations: " << variance << "ns^2" << std::endl;

    const auto sleep_times_std_dev = std::chrono::nanoseconds(
        static_cast<nanoseconds_cnt_t>(round(std::sqrt(variance))));

    std::cout << "[DEBUG] Sleep time standard deviation over "
              << test_iterations << " iterations: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     sleep_times_std_dev)
                     .count()
              << "μs" << std::endl;

    const auto max_sleep_time = std::chrono::nanoseconds{sleep_times.max()};

    std::cout << "[DEBUG] Max sleep time over " << test_iterations
              << " iterations: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     max_sleep_time)
                     .count()
              << "μs" << std::endl;

    const std::chrono::nanoseconds precision_ =
        max_sleep_time + sleep_times_std_dev;

    const std::chrono::milliseconds min_interval_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(precision_ * 10);

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