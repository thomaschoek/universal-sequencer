#ifndef MICRO_COMPOSER_JSON_SERIALIZATION_H
#define MICRO_COMPOSER_JSON_SERIALIZATION_H

#include "gui/event_parameter_traits.h"
#include "sequencable/concepts.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace Micro_composer {
namespace gui {

// Exception thrown when JSON parsing fails
class Json_parse_error : public std::runtime_error {
public:
  explicit Json_parse_error(const std::string& message)
      : std::runtime_error(message) {}
};

// Configuration for JSON parsing limits (DoS protection)
struct Json_parse_config {
  size_t max_nesting_depth{100};      // Maximum nesting level
  size_t max_string_length{10000};    // Maximum string length
  size_t max_array_size{10000};       // Maximum array elements
  size_t max_file_size{100 * 1024 * 1024}; // 100 MB maximum file size
};

/**
 * Serialize sequences to JSON string
 *
 * @param sequences Vector of sequencers, each containing a vector of events
 * @return JSON string representation
 * @throws std::invalid_argument if sequences structure is invalid
 */
template <sequencable::Mut_seq_event Event_t>
std::string sequences_to_json(
    const std::vector<std::vector<Event_t>>& sequences);

/**
 * Deserialize sequences from JSON string with validation and DoS protection
 *
 * @param json_str JSON string to parse
 * @param config Parsing configuration with limits
 * @return Vector of sequencers with their events
 * @throws Json_parse_error if JSON is malformed
 * @throws std::invalid_argument if validation fails
 */
template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> json_to_sequences(
    const std::string& json_str,
    const Json_parse_config& config = Json_parse_config{});

} // namespace gui
} // namespace Micro_composer

#include "gui/json_serialization.tpp"

#endif // MICRO_COMPOSER_JSON_SERIALIZATION_H
