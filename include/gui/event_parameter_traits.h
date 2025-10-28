#ifndef MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_H
#define MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_H

#include "sequencable/premade_samples.h"
#include "sequencable/vector_event.h"
#include <string>
#include <vector>

namespace Micro_composer {

namespace gui {

// Base template - event types should specialize this
template <typename Event_t> struct Event_parameter_traits {
  static constexpr size_t parameter_count = 1;

  static std::string get_parameter_name(size_t idx) { return "param"; }

  static std::string get_parameter_value(const Event_t& event, size_t idx) {
    return "?";
  }

  static void set_parameter_value(Event_t& event, size_t idx,
                                   const std::string& value) {}
};

// Specialization for Premade_samples
template <> struct Event_parameter_traits<sequencable::Premade_samples> {
  using Event_t = sequencable::Premade_samples;

  static constexpr size_t parameter_count = 4;

  static std::string get_parameter_name(size_t idx) {
    static const std::vector<std::string> names = {"Frequency", "Amplitude",
                                                   "Phase", "Duration"};
    return idx < names.size() ? names[idx] : "?";
  }

  static std::string get_parameter_value(const Event_t& event, size_t idx) {
    switch (idx) {
    case 0: // Frequency
      return std::to_string(static_cast<int>(event.frequency)) + " Hz";
    case 1: // Amplitude
      return std::to_string(event.amplitude);
    case 2: // Phase
      return std::to_string(event.phase);
    case 3: // Duration (in milliseconds)
      return std::to_string(
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                     event.duration)
                     .count()) +
             " ms";
    default:
      return "?";
    }
  }

  static void set_parameter_value(Event_t& event, size_t idx,
                                   const std::string& value) {
    try {
      switch (idx) {
      case 0: // Frequency
        event.frequency = std::stod(value);
        event.generate_samples();
        break;
      case 1: // Amplitude
        event.amplitude = std::stod(value);
        event.generate_samples();
        break;
      case 2: // Phase
        event.phase = std::stod(value);
        event.generate_samples();
        break;
      case 3: // Duration (in milliseconds)
        event.duration = std::chrono::milliseconds(std::stoi(value));
        event.generate_samples();
        break;
      }
    } catch (...) {
      // Ignore invalid input
    }
  }
};

// Specialization for Vector_event<double>
template <> struct Event_parameter_traits<sequencable::Vector_event<double>> {
  using Event_t = sequencable::Vector_event<double>;

  static size_t parameter_count(const Event_t& event) {
    return event.data.size();
  }

  static std::string get_parameter_name(size_t idx) {
    return "Data[" + std::to_string(idx) + "]";
  }

  static std::string get_parameter_value(const Event_t& event, size_t idx) {
    if (idx < event.data.size()) {
      return std::to_string(event.data[idx]);
    }
    return "?";
  }

  static void set_parameter_value(Event_t& event, size_t idx,
                                   const std::string& value) {
    try {
      if (idx < event.data.size()) {
        event.data[idx] = std::stod(value);
      }
    } catch (...) {
      // Ignore invalid input
    }
  }
};

} // namespace gui

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_H
