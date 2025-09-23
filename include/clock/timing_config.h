#ifndef MICRO_COMPOSER_TIMING_CONFIG_H
#define MICRO_COMPOSER_TIMING_CONFIG_H

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

#ifdef __linux__
#include <sched.h>
#include <time.h>
#endif

namespace Micro_composer {
namespace timing_capabilities {

constexpr const uint test_iterations{1000};
constexpr const std::chrono::microseconds test_sleep_time{1000}; // 1ms

// Timing configuration based on system capabilities
struct TimingCapabilities {

  std::chrono::nanoseconds precision;
  std::chrono::milliseconds min_interval;
  std::chrono::nanoseconds scheduling_quantum;

private:
  // Detect system scheduling quantum
  static std::chrono::nanoseconds detect_scheduling_quantum() {
#ifdef __linux__
    // First, check what scheduling policy is actually being used
    int policy = sched_getscheduler(0);

#ifndef NDEBUG
    const char* policy_name = "UNKNOWN";
    switch (policy) {
    case SCHED_NORMAL:
      policy_name = "SCHED_NORMAL (CFS)";
      break;
    case SCHED_FIFO:
      policy_name = "SCHED_FIFO";
      break;
    case SCHED_RR:
      policy_name = "SCHED_RR";
      break;
    case SCHED_BATCH:
      policy_name = "SCHED_BATCH";
      break;
    case SCHED_IDLE:
      policy_name = "SCHED_IDLE";
      break;
    default:
      policy_name = "UNKNOWN";
      break;
    }
    std::cout << "[DEBUG] Current scheduling policy: " << policy_name
              << std::endl;
#endif

    // Only query RR interval if we're actually using round-robin
    if (policy == SCHED_RR) {
      struct timespec quantum;
      if (sched_rr_get_interval(0, &quantum) == 0) {
#ifndef NDEBUG
        std::cout
            << "[DEBUG] Using actual RR quantum from sched_rr_get_interval"
            << std::endl;
#endif
        return std::chrono::nanoseconds(quantum.tv_sec * 1'000'000'000 +
                                        quantum.tv_nsec);
      }
    }

    // For other policies, use appropriate granularity values
    switch (policy) {
    case SCHED_NORMAL:
    case SCHED_BATCH:
#ifndef NDEBUG
      std::cout << "[DEBUG] Using CFS typical granularity (4ms)" << std::endl;
#endif
      return std::chrono::milliseconds(4); // CFS typical granularity
    case SCHED_FIFO:
#ifndef NDEBUG
      std::cout << "[DEBUG] SCHED_FIFO has no fixed quantum, using 10ms default"
                << std::endl;
#endif
      return std::chrono::milliseconds(
          10); // FIFO has no quantum, use reasonable default
    case SCHED_IDLE:
#ifndef NDEBUG
      std::cout << "[DEBUG] SCHED_IDLE using 100ms (low priority)" << std::endl;
#endif
      return std::chrono::milliseconds(
          100); // Idle tasks get infrequent scheduling
    default:
#ifndef NDEBUG
      std::cout << "[DEBUG] Unknown policy, using 10ms fallback" << std::endl;
#endif
      return std::chrono::milliseconds(10);
    }
#elif _WIN32
    return std::chrono::milliseconds(15); // Windows typical quantum
#elif __APPLE__
    return std::chrono::milliseconds(10); // macOS typical quantum
#else
    return std::chrono::milliseconds(10); // Generic fallback
#endif
  }

public:
  // Detect system timing capabilities
  TimingCapabilities() {

    typedef std::chrono::nanoseconds::rep nanoseconds_cnt_t;

    // Measure actual sleep precision by testing short sleeps
    std::valarray<nanoseconds_cnt_t> sleep_times(test_iterations);

    for (auto& sleep_time : sleep_times) {
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
    // Detect scheduling quantum
    const std::chrono::nanoseconds scheduling_quantum_ =
        detect_scheduling_quantum();

    // Adjust min_interval based on scheduling quantum if needed
    auto quantum_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        scheduling_quantum_);
    const std::chrono::milliseconds adjusted_min_interval =
        std::max(min_interval_, quantum_ms);

#ifndef NDEBUG
    std::cout << "  Scheduling quantum: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     scheduling_quantum_)
                     .count()
              << "μs" << std::endl;
    if (adjusted_min_interval > min_interval_) {
      std::cout << "  Min interval adjusted from " << min_interval_.count()
                << "ms to " << adjusted_min_interval.count()
                << "ms based on scheduling quantum" << std::endl;
    }
#endif

    precision = precision_;
    min_interval = adjusted_min_interval;
    scheduling_quantum = scheduling_quantum_;
  }

  static const TimingCapabilities& get_instance() {
    static std::once_flag initialized;
    static TimingCapabilities instance;
    std::call_once(initialized, []() {
      // Constructor already does the work
    });
    return instance;
  }
};

} // namespace timing_capabilities
} // namespace Micro_composer

#endif