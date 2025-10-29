#ifndef MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_MIDI_H
#define MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_MIDI_H

#include "gui/event_parameter_traits.h"
#include "sequencable/midi_event.h"
#include <cstdio>
#include <stdexcept>
#include <string>

namespace Micro_composer {

namespace gui {

// Specialization for Midi_event
template <> struct Event_parameter_traits<sequencable::Midi_event> {
  using Event_t = sequencable::Midi_event;

  static constexpr size_t parameter_count = 5;

  static std::string get_parameter_name(size_t idx) {
    static const std::vector<std::string> names = {
        "Enabled", "Duration (ms)", "Channel (0-15)", "Note (0-127)",
        "Velocity (0-127)"};
    return idx < names.size() ? names[idx] : "Unknown";
  }

  static std::string get_parameter_value(const Event_t& event, size_t idx) {
    char buffer[32];
    switch (idx) {
    case 0: // Enabled
      return event.enabled ? "1" : "0";
    case 1: // Duration (milliseconds)
      snprintf(buffer, sizeof(buffer), "%ld",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   event.duration)
                   .count());
      return buffer;
    case 2: // Channel
      snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(event.channel));
      return buffer;
    case 3: // Note (show as name if possible)
      return Event_t::note_to_name(event.note) + " (" +
             std::to_string(static_cast<unsigned>(event.note)) + ")";
    case 4: // Velocity
      snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(event.velocity));
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
    case 1: { // Duration (milliseconds)
      long ms = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument(
            "Invalid duration format. Expected: number (milliseconds)");
      }
      if (ms <= 0) {
        throw std::invalid_argument("Duration must be positive");
      }
      event.duration = std::chrono::milliseconds(ms);
      break;
    }
    case 2: { // Channel
      long ch = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument("Invalid channel format. Expected: number");
      }
      if (ch < 0 || ch > 15) {
        throw std::invalid_argument("Channel must be between 0 and 15");
      }
      event.channel = static_cast<uint8_t>(ch);
      break;
    }
    case 3: { // Note (accept number or name)
      // Try parsing as note name first
      try {
        event.note = Event_t::name_to_note(value);
        break;
      } catch (...) {
        // Fall through to numeric parsing
      }
      // Try parsing as number
      long note = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument(
            "Invalid note format. Expected: note name (e.g., C4) or number "
            "(0-127)");
      }
      if (note < 0 || note > 127) {
        throw std::invalid_argument("Note must be between 0 and 127");
      }
      event.note = static_cast<uint8_t>(note);
      break;
    }
    case 4: { // Velocity
      long vel = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0') {
        throw std::invalid_argument("Invalid velocity format. Expected: number");
      }
      if (vel < 0 || vel > 127) {
        throw std::invalid_argument("Velocity must be between 0 and 127");
      }
      event.velocity = static_cast<uint8_t>(vel);
      break;
    }
    default:
      throw std::invalid_argument("Unknown parameter");
    }
  }
};

} // namespace gui

} // namespace Micro_composer

#endif // MICRO_COMPOSER_EVENT_PARAMETER_TRAITS_MIDI_H
