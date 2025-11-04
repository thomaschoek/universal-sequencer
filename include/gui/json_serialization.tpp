#ifndef MICRO_COMPOSER_JSON_SERIALIZATION_TPP
#define MICRO_COMPOSER_JSON_SERIALIZATION_TPP

#include "gui/json_serialization.h"
#include <sstream>

namespace Micro_composer {
namespace gui {

// Temporary implementation using old hand-rolled parser
// This will be replaced with nlohmann/json in the next commit

template <sequencable::Mut_seq_event Event_t>
std::string sequences_to_json(
    const std::vector<std::vector<Event_t>>& sequences) {
  using Traits = Event_parameter_traits<Event_t>;

  std::ostringstream json;
  json << "{\n  \"sequencers\": [\n";

  for (size_t seq_idx = 0; seq_idx < sequences.size(); ++seq_idx) {
    if (seq_idx > 0) json << ",\n";
    json << "    {\n";
    json << "      \"events\": [\n";

    const auto& events = sequences[seq_idx];
    for (size_t evt_idx = 0; evt_idx < events.size(); ++evt_idx) {
      if (evt_idx > 0) json << ",\n";
      json << "        {\n";

      const auto& event = events[evt_idx];
      constexpr size_t num_params = Traits::parameter_count;

      for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
        if (param_idx > 0) json << ",\n";
        std::string param_name = Traits::get_parameter_name(param_idx);
        std::string param_value = Traits::get_parameter_value(event, param_idx);

        // Escape quotes in values
        size_t pos = 0;
        while ((pos = param_value.find('"', pos)) != std::string::npos) {
          param_value.insert(pos, "\\");
          pos += 2;
        }

        json << "          \"" << param_name << "\": \"" << param_value << "\"";
      }

      json << "\n        }";
    }

    json << "\n      ]\n";
    json << "    }";
  }

  json << "\n  ]\n}\n";
  return json.str();
}

template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> json_to_sequences(
    const std::string& json_str,
    const Json_parse_config& config) {
  using Traits = Event_parameter_traits<Event_t>;

  // TEMPORARY: Old hand-rolled parser (has security issues!)
  // This will be replaced with nlohmann/json

  // Basic file size check
  if (json_str.size() > config.max_file_size) {
    throw Json_parse_error("JSON file exceeds maximum size limit");
  }

  std::istringstream input(json_str);
  std::string line;

  std::vector<std::vector<Event_t>> new_sequences;
  std::vector<Event_t> current_sequence;
  Event_t current_event{};
  bool in_event = false;

  while (std::getline(input, line)) {
    // Strip whitespace
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    if (line == "{" && !in_event) {
      in_event = true;
      current_event = Event_t{};
      continue;
    }

    if (line == "}," || line == "}") {
      if (in_event) {
        current_sequence.push_back(current_event);
        in_event = false;
      }
      continue;
    }

    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos && in_event) {
      std::string param_part = line.substr(0, colon_pos);
      std::string value_part = line.substr(colon_pos + 1);

      auto remove_quotes = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\""));
        s.erase(s.find_last_not_of(" \t\",") + 1);
      };

      remove_quotes(param_part);
      remove_quotes(value_part);

      constexpr size_t num_params = Traits::parameter_count;
      for (size_t i = 0; i < num_params; ++i) {
        if (Traits::get_parameter_name(i) == param_part) {
          try {
            Traits::set_parameter_value(current_event, i, value_part);
          } catch (const std::exception& e) {
            throw Json_parse_error(std::string("Error parsing parameter '") +
                                   param_part + "': " + e.what());
          }
          break;
        }
      }
    }

    if (line.find("\"events\":") != std::string::npos &&
        !current_sequence.empty()) {
      new_sequences.push_back(current_sequence);
      current_sequence.clear();
    }
  }

  if (!current_sequence.empty()) {
    new_sequences.push_back(current_sequence);
  }

  return new_sequences;
}

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_JSON_SERIALIZATION_TPP
