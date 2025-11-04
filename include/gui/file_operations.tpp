#ifndef MICRO_COMPOSER_FILE_OPERATIONS_TPP
#define MICRO_COMPOSER_FILE_OPERATIONS_TPP

#include "gui/file_operations.h"
#include <fstream>
#include <sstream>

namespace Micro_composer {
namespace gui {

// TEMPORARY: Insecure implementation without path validation
// This will be replaced with std::filesystem in the next commit

template <sequencable::Mut_seq_event Event_t>
void save_sequences_to_file(
    const std::string& filepath,
    const std::vector<std::vector<Event_t>>& sequences,
    const File_operation_config& config) {

  // VULNERABILITY: No path validation!
  // Allows: ../../../etc/passwd, symlinks, etc.

  // Serialize to JSON
  std::string json_str = sequences_to_json(sequences);

  // Check size limit
  if (json_str.size() > config.max_file_size) {
    throw File_operation_error("Data exceeds maximum file size of " +
                               std::to_string(config.max_file_size) + " bytes");
  }

  // Open file (INSECURE: no path validation)
  std::ofstream file(filepath);
  if (!file.is_open()) {
    throw File_operation_error("Failed to open file for writing: " + filepath);
  }

  file << json_str;
  file.close();

  if (file.fail()) {
    throw File_operation_error("Failed to write to file: " + filepath);
  }
}

template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> load_sequences_from_file(
    const std::string& filepath,
    const File_operation_config& config) {

  // VULNERABILITY: No path validation!
  // Allows: ../../../etc/passwd, symlinks, etc.

  // Open file (INSECURE: no path validation)
  std::ifstream file(filepath);
  if (!file.is_open()) {
    throw File_operation_error("Failed to open file for reading: " + filepath);
  }

  // Read entire file
  std::ostringstream buffer;
  buffer << file.rdbuf();
  file.close();

  std::string json_str = buffer.str();

  // Check size limit
  if (json_str.size() > config.max_file_size) {
    throw File_operation_error("File exceeds maximum size of " +
                               std::to_string(config.max_file_size) + " bytes");
  }

  // Parse JSON with validation
  Json_parse_config json_config;
  json_config.max_file_size = config.max_file_size;

  return json_to_sequences<Event_t>(json_str, json_config);
}

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_FILE_OPERATIONS_TPP
