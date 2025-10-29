#ifndef MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_H
#define MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_H

#include "sequencable/premade_samples.h"
#include "sequencable/vector_event.h"
#include <cstdio>
#include <stdexcept>
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

  static constexpr size_t parameter_count = 6;

  static std::string get_parameter_name(size_t idx) {
    static const std::vector<std::string> names = {
        "Enabled", "Offset (ms)", "Duration (ms)", "Frequency (Hz)",
        "Amplitude (0-1)", "Phase (rad)"};
    return idx < names.size() ? names[idx] : "Unknown";
  }

  static std::string get_parameter_value(const Event_t& event, size_t idx) {
    char buffer[32];
    switch (idx) {
    case 0: // Enabled
      return event.enabled ? "1" : "0";
    case 1: // Offset (milliseconds)
      snprintf(buffer, sizeof(buffer), "%ld",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   event.offset)
                   .count());
      return buffer;
    case 2: // Duration (milliseconds)
      snprintf(buffer, sizeof(buffer), "%ld",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   event.duration)
                   .count());
      return buffer;
    case 3: // Frequency
      snprintf(buffer, sizeof(buffer), "%.2f", event.frequency);
      return buffer;
    case 4: // Amplitude
      snprintf(buffer, sizeof(buffer), "%.3f", event.amplitude);
      return buffer;
    case 5: // Phase
      snprintf(buffer, sizeof(buffer), "%.3f", event.phase);
      return buffer;
    default:
      return "";
    }
  }

  static void set_parameter_value(Event_t& event, size_t idx,
                                   const std::string& value) {
    char* end;
    switch (idx) {
    case 0: { // Enabled
      if (value != "0" && value != "1") {
        throw std::invalid_argument("Enabled must be 0 or 1");
      }
      event.enabled = (value == "1");
      break;
    }
    case 1: { // Offset (milliseconds)
      long ms = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument(
            "Invalid offset format. Expected: number (milliseconds)");
      }
      if (ms < 0) {
        throw std::invalid_argument("Offset cannot be negative");
      }
      event.offset = std::chrono::milliseconds(ms);
      break;
    }
    case 2: { // Duration (milliseconds)
      long ms = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument(
            "Invalid duration format. Expected: number (milliseconds)");
      }
      if (ms <= 0) {
        throw std::invalid_argument("Duration must be positive");
      }
      event.duration = std::chrono::milliseconds(ms);
      event.generate_samples();
      break;
    }
    case 3: { // Frequency
      double freq = std::strtod(value.c_str(), &end);
      if (end == value.c_str()) {
        throw std::invalid_argument("Invalid frequency format. Expected: number");
      }
      if (freq <= 0.0 || freq > 20000.0) {
        throw std::invalid_argument("Frequency must be between 0 and 20000 Hz");
      }
      event.frequency = freq;
      event.generate_samples();
      break;
    }
    case 4: { // Amplitude
      double amp = std::strtod(value.c_str(), &end);
      if (end == value.c_str()) {
        throw std::invalid_argument("Invalid amplitude format. Expected: number");
      }
      if (amp < 0.0 || amp > 1.0) {
        throw std::invalid_argument("Amplitude must be between 0.0 and 1.0");
      }
      event.amplitude = amp;
      event.generate_samples();
      break;
    }
    case 5: { // Phase
      double phase = std::strtod(value.c_str(), &end);
      if (end == value.c_str()) {
        throw std::invalid_argument("Invalid phase format. Expected: number");
      }
      event.phase = phase;
      event.generate_samples();
      break;
    }
    default:
      throw std::invalid_argument("Unknown parameter");
    }
  }
};

// Note: For Midi_event specialization, include "gui/event_parameter_traits_midi.h"

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
