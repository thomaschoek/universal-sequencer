#ifndef MICRO_COMPOSER_JSON_SERIALIZATION_TPP
#define MICRO_COMPOSER_JSON_SERIALIZATION_TPP

#include "gui/json_serialization.h"
#include <nlohmann/json.hpp>

namespace Micro_composer {
namespace gui {

// Implementation using nlohmann/json for proper security and validation

template <sequencable::Mut_seq_event Event_t>
std::string sequences_to_json(
    const std::vector<std::vector<Event_t>>& sequences) {
  using Traits = Event_parameter_traits<Event_t>;
  using json = nlohmann::json;

  json root;
  json sequencers_array = json::array();

  for (const auto& sequence : sequences) {
    json sequencer_obj;
    json events_array = json::array();

    for (const auto& event : sequence) {
      json event_obj;
      constexpr size_t num_params = Traits::parameter_count;

      for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
        std::string param_name = Traits::get_parameter_name(param_idx);
        std::string param_value = Traits::get_parameter_value(event, param_idx);
        event_obj[param_name] = param_value;
      }

      events_array.push_back(event_obj);
    }

    sequencer_obj["events"] = events_array;
    sequencers_array.push_back(sequencer_obj);
  }

  root["sequencers"] = sequencers_array;

  // Serialize with indentation for readability
  return root.dump(2);
}

template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> json_to_sequences(
    const std::string& json_str,
    const Json_parse_config& config) {
  using Traits = Event_parameter_traits<Event_t>;
  using json = nlohmann::json;

  // Validate file size before parsing
  if (json_str.size() > config.max_file_size) {
    throw Json_parse_error("JSON file exceeds maximum size limit of " +
                           std::to_string(config.max_file_size) + " bytes");
  }

  // Parse JSON with exception handling
  json root;
  try {
    root = json::parse(json_str);
  } catch (const json::parse_error& e) {
    throw Json_parse_error(std::string("JSON parse error: ") + e.what());
  }

  // Validate structure
  if (!root.is_object()) {
    throw Json_parse_error("JSON root must be an object");
  }

  if (!root.contains("sequencers")) {
    throw Json_parse_error("JSON missing 'sequencers' field");
  }

  if (!root["sequencers"].is_array()) {
    throw Json_parse_error("'sequencers' field must be an array");
  }

  const auto& sequencers = root["sequencers"];

  // Validate array size
  if (sequencers.size() > config.max_array_size) {
    throw Json_parse_error("Too many sequencers: " +
                           std::to_string(sequencers.size()) +
                           " exceeds limit of " +
                           std::to_string(config.max_array_size));
  }

  std::vector<std::vector<Event_t>> result;

  for (size_t seq_idx = 0; seq_idx < sequencers.size(); ++seq_idx) {
    const auto& sequencer = sequencers[seq_idx];

    if (!sequencer.is_object()) {
      throw Json_parse_error("Sequencer at index " +
                             std::to_string(seq_idx) + " must be an object");
    }

    if (!sequencer.contains("events")) {
      throw Json_parse_error("Sequencer at index " +
                             std::to_string(seq_idx) +
                             " missing 'events' field");
    }

    if (!sequencer["events"].is_array()) {
      throw Json_parse_error("'events' field at sequencer " +
                             std::to_string(seq_idx) + " must be an array");
    }

    const auto& events = sequencer["events"];

    // Validate events array size
    if (events.size() > config.max_array_size) {
      throw Json_parse_error("Too many events in sequencer " +
                             std::to_string(seq_idx) + ": " +
                             std::to_string(events.size()) +
                             " exceeds limit of " +
                             std::to_string(config.max_array_size));
    }

    std::vector<Event_t> sequence;

    for (size_t evt_idx = 0; evt_idx < events.size(); ++evt_idx) {
      const auto& event_obj = events[evt_idx];

      if (!event_obj.is_object()) {
        throw Json_parse_error("Event at index " + std::to_string(evt_idx) +
                               " in sequencer " + std::to_string(seq_idx) +
                               " must be an object");
      }

      Event_t event{};
      constexpr size_t num_params = Traits::parameter_count;

      // Parse each parameter
      for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
        std::string param_name = Traits::get_parameter_name(param_idx);

        if (event_obj.contains(param_name)) {
          // Get value as string
          std::string value_str;

          const auto& value = event_obj[param_name];
          if (value.is_string()) {
            value_str = value.get<std::string>();

            // Validate string length
            if (value_str.size() > config.max_string_length) {
              throw Json_parse_error(
                  "String value for parameter '" + param_name +
                  "' exceeds maximum length of " +
                  std::to_string(config.max_string_length) + " characters");
            }
          } else {
            // Convert non-string values to string
            value_str = value.dump();
          }

          // Set parameter value
          try {
            Traits::set_parameter_value(event, param_idx, value_str);
          } catch (const std::exception& e) {
            throw Json_parse_error("Error setting parameter '" + param_name +
                                   "' at event " + std::to_string(evt_idx) +
                                   " in sequencer " + std::to_string(seq_idx) +
                                   ": " + e.what());
          }
        }
        // Note: Missing parameters keep their default values
      }

      sequence.push_back(event);
    }

    result.push_back(sequence);
  }

  return result;
}

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_JSON_SERIALIZATION_TPP
