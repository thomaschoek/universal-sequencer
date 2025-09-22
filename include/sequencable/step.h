#ifndef MICRO_COMPOSER_STEP_H
#define MICRO_COMPOSER_STEP_H

#include <chrono>
#include <vector>

#include "sequencable/sequencable.h"

namespace MicroComposer {

namespace sequencable {

struct Step {
  typedef std::vector<double>::size_type param_idx_t;
  std::vector<double> parameters;
  std::chrono::duration<double> offset{std::chrono::duration<double>(0)};
  std::chrono::duration<double> duration{std::chrono::duration<double>(1)};

  Step() = default;
  Step(double offset_seconds, double length_seconds,
       const std::vector<double> &params = {})
      : parameters(params),
        offset(std::chrono::duration<double>(offset_seconds)),
        duration(std::chrono::duration<double>(length_seconds)) {}
  Step(double offset_seconds, double length_seconds,
       std::vector<double> &&params)
      : parameters(std::move(params)),
        offset(std::chrono::duration<double>(offset_seconds)),
        duration(std::chrono::duration<double>(length_seconds)) {}
#ifndef NDEBUG
  friend std::ostream &operator<<(std::ostream &os, const Step &step) {
    os << "Step(offset: " << step.offset.count()
       << "s, length: " << step.duration.count() << "s, parameters: [";
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

static_assert(Sequencable<Step>, "Step does not satisfy Sequencable concept");

} // namespace sequencable

} // namespace MicroComposer

#endif // STEP_H
