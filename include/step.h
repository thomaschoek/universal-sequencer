#ifndef STEP_H
#define STEP_H

#include <chrono>

namespace sequencer {

struct Step {
  typedef std::vector<double>::size_type param_idx_t;
  std::vector<double> parameters;
  std::chrono::duration<double> offset{std::chrono::duration<double>(0)};
  std::chrono::duration<double> length{std::chrono::duration<double>(1)};
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
