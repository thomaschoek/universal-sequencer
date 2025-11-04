#ifndef MICRO_COMPOSER_FILE_OPERATIONS_TPP
#define MICRO_COMPOSER_FILE_OPERATIONS_TPP

#include "gui/file_operations.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Micro_composer {
namespace gui {

// Secure implementation using std::filesystem for path validation

namespace {

// Helper function to validate and canonicalize path
std::filesystem::path validate_path(
    const std::string& filepath,
    const File_operation_config& config) {

  if (!config.validate_path) {
    return filepath; // Skip validation if disabled
  }

  namespace fs = std::filesystem;

  // Convert to filesystem path
  fs::path path(filepath);

  // Check if path exists (for reading) or parent exists (for writing)
  fs::path check_path = path;
  if (!fs::exists(path) && path.has_parent_path()) {
    check_path = path.parent_path();
  }

  // Canonicalize path (resolves .., symlinks, etc.)
  fs::path canonical_path;
  try {
    if (fs::exists(check_path)) {
      canonical_path = fs::canonical(check_path);
      // If original path didn't exist, append the filename
      if (!fs::exists(path) && path.has_filename()) {
        canonical_path /= path.filename();
      }
    } else {
      // Path doesn't exist, try to construct canonical path from parts
      canonical_path = fs::absolute(path);
      // Normalize by removing . and ..
      canonical_path = canonical_path.lexically_normal();
    }
  } catch (const fs::filesystem_error& e) {
    throw File_operation_error("Invalid file path: " + filepath +
                               " (" + e.what() + ")");
  }

  // Validate symlinks if not allowed
  if (!config.allow_symlinks) {
    // Use symlink_status to check the path itself, not its target
    std::error_code ec;
    auto status = fs::symlink_status(path, ec);
    if (!ec && fs::is_symlink(status)) {
      throw File_operation_error("Symlinks are not allowed: " + filepath);
    }

    // Also check parent directories for symlinks
    fs::path check = path.parent_path();
    while (check.has_parent_path() && check != check.root_path()) {
      auto parent_status = fs::symlink_status(check, ec);
      if (!ec && fs::is_symlink(parent_status)) {
        throw File_operation_error(
            "Path contains symlink in parent directory: " + filepath);
      }
      check = check.parent_path();
    }
  }

  // Validate against allowed directory if specified
  if (!config.allowed_directory.empty()) {
    fs::path allowed_canonical;
    try {
      allowed_canonical = fs::canonical(config.allowed_directory);
    } catch (const fs::filesystem_error& e) {
      throw File_operation_error("Invalid allowed directory: " +
                                 config.allowed_directory + " (" + e.what() + ")");
    }

    // Check if canonical path is within allowed directory
    auto [path_it, allowed_it] =
        std::mismatch(canonical_path.begin(), canonical_path.end(),
                      allowed_canonical.begin(), allowed_canonical.end());

    // Path must have allowed_canonical as prefix
    if (allowed_it != allowed_canonical.end()) {
      throw File_operation_error(
          "Path is outside allowed directory: " + filepath +
          " (resolved to: " + canonical_path.string() +
          ", allowed: " + allowed_canonical.string() + ")");
    }
  }

  // Additional security: block access to sensitive system paths
  const std::string path_str = canonical_path.string();
  const std::vector<std::string> forbidden_paths = {
      "/etc/passwd", "/etc/shadow", "/etc/sudoers",
      "/root", "/boot"
  };

  for (const auto& forbidden : forbidden_paths) {
    if (path_str.find(forbidden) == 0) {
      throw File_operation_error("Access to sensitive system path denied: " +
                                 path_str);
    }
  }

  return canonical_path;
}

} // anonymous namespace

template <sequencable::Mut_seq_event Event_t>
void save_sequences_to_file(
    const std::string& filepath,
    const std::vector<std::vector<Event_t>>& sequences,
    const File_operation_config& config) {

  // Validate path with security checks
  std::filesystem::path validated_path = validate_path(filepath, config);

  // Serialize to JSON
  std::string json_str = sequences_to_json(sequences);

  // Check size limit
  if (json_str.size() > config.max_file_size) {
    throw File_operation_error("Data exceeds maximum file size of " +
                               std::to_string(config.max_file_size) + " bytes");
  }

  // Ensure parent directory exists
  if (validated_path.has_parent_path()) {
    std::filesystem::path parent = validated_path.parent_path();
    if (!std::filesystem::exists(parent)) {
      throw File_operation_error("Parent directory does not exist: " +
                                 parent.string());
    }
  }

  // Open file with validated path
  std::ofstream file(validated_path);
  if (!file.is_open()) {
    throw File_operation_error("Failed to open file for writing: " +
                               validated_path.string());
  }

  file << json_str;
  file.close();

  if (file.fail()) {
    throw File_operation_error("Failed to write to file: " +
                               validated_path.string());
  }
}

template <sequencable::Mut_seq_event Event_t>
std::vector<std::vector<Event_t>> load_sequences_from_file(
    const std::string& filepath,
    const File_operation_config& config) {

  // Validate path with security checks
  std::filesystem::path validated_path = validate_path(filepath, config);

  // Check if file exists
  if (!std::filesystem::exists(validated_path)) {
    throw File_operation_error("File does not exist: " +
                               validated_path.string());
  }

  // Check file size before reading
  std::uintmax_t file_size = std::filesystem::file_size(validated_path);
  if (file_size > config.max_file_size) {
    throw File_operation_error("File exceeds maximum size of " +
                               std::to_string(config.max_file_size) +
                               " bytes: " + std::to_string(file_size) + " bytes");
  }

  // Open file with validated path
  std::ifstream file(validated_path);
  if (!file.is_open()) {
    throw File_operation_error("Failed to open file for reading: " +
                               validated_path.string());
  }

  // Read entire file
  std::ostringstream buffer;
  buffer << file.rdbuf();
  file.close();

  std::string json_str = buffer.str();

  // Parse JSON with validation
  Json_parse_config json_config;
  json_config.max_file_size = config.max_file_size;

  return json_to_sequences<Event_t>(json_str, json_config);
}

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_FILE_OPERATIONS_TPP
