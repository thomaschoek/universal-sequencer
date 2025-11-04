#ifndef MICRO_COMPOSER_FILE_OPERATIONS_H
#define MICRO_COMPOSER_FILE_OPERATIONS_H

#include "gui/json_serialization.h"
#include <string>
#include <vector>

namespace Micro_composer {
namespace gui {

// Exception thrown when file operations fail
class File_operation_error : public std::runtime_error {
public:
  explicit File_operation_error(const std::string& message)
      : std::runtime_error(message) {}
};

// Configuration for file operations (security limits)
struct File_operation_config {
  size_t max_file_size{100 * 1024 * 1024}; // 100 MB
  std::string allowed_directory;            // If set, restricts to this directory
  bool allow_symlinks{false};               // Whether to follow symlinks
  bool validate_path{true};                 // Whether to validate paths
};

/**
 * Save sequences to file with path validation
 *
 * @param filepath Path to save file
 * @param sequences Vector of sequencers with events
 * @param config File operation configuration
 * @throws File_operation_error if path is invalid or operation fails
 */
template <sequencable::Mut_seq_event Event_t>
void save_sequences_to_file(
    const std::string& filepath,
    const std::vector<std::vector<Event_t>>& sequences,
    const File_operation_config& config = File_operation_config{});

/**
 * Load sequences from file with path validation
 *
 * @param filepath Path to load file from
 * @param config File operation configuration
 * @return Vector of sequencers with their events
 * @throws File_operation_error if path is invalid or operation fails
 */
template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> load_sequences_from_file(
    const std::string& filepath,
    const File_operation_config& config = File_operation_config{});

} // namespace gui
} // namespace Micro_composer

#include "gui/file_operations.tpp"

#endif // MICRO_COMPOSER_FILE_OPERATIONS_H
