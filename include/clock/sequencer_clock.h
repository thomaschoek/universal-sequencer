#ifndef MICRO_COMPOSER_VARIABLE_CLOCK_H
#define MICRO_COMPOSER_VARIABLE_CLOCK_H

#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>
#include <vector>

namespace Micro_composer {

namespace sequencer {

namespace transport {

class Sequencer_transport {
public:
  using Clock = std::chrono::steady_clock;
  using Time_point = Clock::time_point;
  using Duration = Clock::duration;
  using Intervals = std::vector<Duration>;
  using Size_type = Intervals::size_type;
  using Iterator = Intervals::iterator;
  using Initializer_list = std::initializer_list<Duration>;
  using Handler = std::function<void()>;

  Sequencer_transport() = default;
  explicit Sequencer_transport(Handler handler,
                               Initializer_list durations = {});

  // Thread-safe transport control
  void start(const Time_point start_time = Clock::now());
  void pause(const Time_point pause_time = Clock::now());
  void stop(const Time_point stop_time = Clock::now());
  bool is_running() const;

  // Set the handler function to be called on each tick
  void set_handler(const std::function<void()>& handler);

  // Duration container methods
  void assign(Size_type, const Duration&);
  void push_back(const Duration&);
  void insert(Size_type, const Duration&);
  void erase(Size_type);
  void assign(Initializer_list);
  void assign(const std::vector<Duration>&);

private:
  void run(std::stop_token,
           const Time_point initial_tick = Clock::now() + min_duration_ +
                                           busy_wait_,
           const Size_type initial_i = 0);
  std::jthread runner_;
  Intervals intervals_;
  std::atomic<Iterator> interval_itr_{intervals_.begin()};
  std::atomic<Time_point> next_tick_;
  Handler handler_ = []() {};
  std::mutex mutex_;
  static constexpr const Duration min_duration_{std::chrono::milliseconds(10)};
  static constexpr const Duration busy_wait_{std::chrono::milliseconds(5)};
};

} // namespace transport

} // namespace sequencer

} // namespace Micro_composer

#endif
