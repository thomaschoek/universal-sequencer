#ifndef STEP_H
#define STEP_H

#include <chrono>
#include <vector>

namespace sequencer {

struct Step {
  typedef std::vector<double>::size_type param_idx_t;
  std::vector<double> parameters;
  std::chrono::duration<double> offset{std::chrono::duration<double>(0)};
  std::chrono::duration<double> length{std::chrono::duration<double>(1)};

  Step() = default;
  Step(double offset_seconds, double length_seconds,
       const std::vector<double>& params = {})
      : parameters(params),
        offset(std::chrono::duration<double>(offset_seconds)),
        length(std::chrono::duration<double>(length_seconds)) {}
  Step(double offset_seconds, double length_seconds,
       std::vector<double>&& params)
      : parameters(std::move(params)),
        offset(std::chrono::duration<double>(offset_seconds)),
        length(std::chrono::duration<double>(length_seconds)) {}
#ifndef NDEBUG
  friend std::ostream &operator<<(std::ostream &os, const Step &step) {
    os << "Step(offset: " << step.offset.count()
       << "s, length: " << step.length.count() << "s, parameters: [";
    for (size_t i = 0; i < step.parameters.size(); ++i) {
      os << step.parameters[i];
      if (i < step.parameters.size() - 1) {
        os << ", ";
      }
    }
    os << "])";
    return os;
  }
#endif
};

} // namespace sequencer

#endif // STEP_H
