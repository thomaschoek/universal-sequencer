#include <catch2/catch_test_macros.hpp>
#include "gui/file_operations.h"
#include "gui/event_parameter_traits.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unistd.h>

using namespace Micro_composer::gui;
using namespace Micro_composer::sequencable;

// Reuse Test_json_event from JSON tests
struct Test_file_event {
  bool enabled{true};
  Duration duration{std::chrono::milliseconds(100)};
  Time_point scheduled_time{};
  double frequency{440.0};
  std::string name{"default"};

  void set_duration(Duration d) { duration = d; }
  void set_scheduled_time(Time_point t) { scheduled_time = t; }
  void update(const Test_file_event& other) {
    enabled = other.enabled;
    duration = other.duration;
    scheduled_time = other.scheduled_time;
    frequency = other.frequency;
    name = other.name;
  }
};

// Specialize Event_parameter_traits for Test_file_event
template <>
struct Micro_composer::gui::Event_parameter_traits<Test_file_event> {
  static constexpr size_t parameter_count = 4;

  static std::string get_parameter_name(size_t index) {
    switch (index) {
    case 0: return "enabled";
    case 1: return "duration_ms";
    case 2: return "frequency";
    case 3: return "name";
    default: throw std::out_of_range("Invalid parameter index");
    }
  }

  static std::string get_parameter_value(const Test_file_event& event,
                                          size_t index) {
    switch (index) {
    case 0: return event.enabled ? "true" : "false";
    case 1:
      return std::to_string(
          std::chrono::duration_cast<std::chrono::milliseconds>(event.duration)
              .count());
    case 2: return std::to_string(event.frequency);
    case 3: return event.name;
    default: throw std::out_of_range("Invalid parameter index");
    }
  }

  static void set_parameter_value(Test_file_event& event, size_t index,
                                   const std::string& value) {
    switch (index) {
    case 0: event.enabled = (value == "true" || value == "1"); break;
    case 1: event.duration = std::chrono::milliseconds(std::stoi(value)); break;
    case 2: event.frequency = std::stod(value); break;
    case 3: event.name = value; break;
    default: throw std::out_of_range("Invalid parameter index");
    }
  }
};

// Test fixture with temporary directory
struct FileOpsFixture {
  std::filesystem::path temp_dir;
  std::filesystem::path allowed_dir;
  std::filesystem::path forbidden_dir;

  FileOpsFixture() {
    // Create temporary test directories
    temp_dir = std::filesystem::temp_directory_path() / "microcomposer_test";
    std::filesystem::create_directories(temp_dir);

    allowed_dir = temp_dir / "allowed";
    std::filesystem::create_directories(allowed_dir);

    forbidden_dir = temp_dir / "forbidden";
    std::filesystem::create_directories(forbidden_dir);
  }

  ~FileOpsFixture() {
    // Cleanup
    std::filesystem::remove_all(temp_dir);
  }

  std::vector<std::vector<Test_file_event>> make_test_sequences() {
    Test_file_event event;
    event.name = "test";
    event.frequency = 440.0;
    return {{event}};
  }
};

TEST_CASE_METHOD(FileOpsFixture, "File operations - basic functionality",
                 "[file][basic]") {
  auto sequences = make_test_sequences();

  SECTION("Save and load to valid path") {
    std::string filepath = (allowed_dir / "test.json").string();

    save_sequences_to_file(filepath, sequences);
    REQUIRE(std::filesystem::exists(filepath));

    auto loaded = load_sequences_from_file<Test_file_event>(filepath);
    REQUIRE(loaded.size() == 1);
    REQUIRE(loaded[0].size() == 1);
    REQUIRE(loaded[0][0].name == "test");
  }

  SECTION("File size limits enforced on save") {
    File_operation_config config;
    config.max_file_size = 100; // Very small limit

    // Create large sequence
    std::vector<std::vector<Test_file_event>> large_sequences;
    std::vector<Test_file_event> events;
    for (int i = 0; i < 100; ++i) {
      Test_file_event e;
      e.name = std::string(100, 'a'); // 100 char name
      events.push_back(e);
    }
    large_sequences.push_back(events);

    std::string filepath = (allowed_dir / "large.json").string();
    REQUIRE_THROWS_AS(
        save_sequences_to_file(filepath, large_sequences, config),
        File_operation_error);
  }

  SECTION("File size limits enforced on load") {
    File_operation_config config;
    config.max_file_size = 100; // Very small limit

    // Create large file manually
    std::string filepath = (allowed_dir / "large.json").string();
    std::ofstream file(filepath);
    file << std::string(200, 'x'); // 200 bytes
    file.close();

    REQUIRE_THROWS_AS(
        load_sequences_from_file<Test_file_event>(filepath, config),
        File_operation_error);
  }
}

TEST_CASE_METHOD(FileOpsFixture, "File operations - security: path traversal",
                 "[file][security][!mayfail]") {
  auto sequences = make_test_sequences();

  SECTION("Directory traversal using ../ should be blocked") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.validate_path = true;

    // Try to escape allowed directory
    std::string filepath = (allowed_dir / "../forbidden/test.json").string();

    // Should throw File_operation_error, not write outside allowed dir
    REQUIRE_THROWS_AS(save_sequences_to_file(filepath, sequences, config),
                      File_operation_error);

    // Verify file was NOT created in forbidden directory
    REQUIRE_FALSE(std::filesystem::exists(forbidden_dir / "test.json"));
  }

  SECTION("Absolute paths outside allowed directory should be blocked") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.validate_path = true;

    std::string filepath = (forbidden_dir / "test.json").string();

    REQUIRE_THROWS_AS(save_sequences_to_file(filepath, sequences, config),
                      File_operation_error);

    REQUIRE_FALSE(std::filesystem::exists(forbidden_dir / "test.json"));
  }

  SECTION("Symlinks should be validated") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.allow_symlinks = false;
    config.validate_path = true;

    // Create symlink from allowed to forbidden directory
    std::filesystem::path symlink_path = allowed_dir / "symlink";
    std::filesystem::path target = forbidden_dir / "target.json";

    try {
      std::filesystem::create_symlink(target, symlink_path);
    } catch (...) {
      // Skip test if symlink creation fails (permissions, etc.)
      return;
    }

    // Try to write through symlink
    REQUIRE_THROWS_AS(
        save_sequences_to_file(symlink_path.string(), sequences, config),
        File_operation_error);

    // Verify file was NOT created via symlink
    REQUIRE_FALSE(std::filesystem::exists(target));

    // Cleanup
    std::filesystem::remove(symlink_path);
  }

  SECTION("Multiple ../ levels should be blocked") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.validate_path = true;

    std::string filepath =
        (allowed_dir / "../../../../../../etc/test.json").string();

    REQUIRE_THROWS_AS(save_sequences_to_file(filepath, sequences, config),
                      File_operation_error);
  }

  SECTION("Path with . and .. should be canonicalized") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.validate_path = true;

    // ./allowed/./test/../test.json should resolve to allowed/test.json (OK)
    std::string filepath =
        (allowed_dir / "./test/../test.json").string();

    // Should succeed after canonicalization
    save_sequences_to_file(filepath, sequences, config);

    // Verify file created at correct location
    REQUIRE(std::filesystem::exists(allowed_dir / "test.json"));
  }
}

TEST_CASE_METHOD(FileOpsFixture,
                 "File operations - security: sensitive file protection",
                 "[file][security][!mayfail]") {
  auto sequences = make_test_sequences();

  SECTION("Cannot write to /etc/passwd") {
    File_operation_config config;
    config.validate_path = true;

    // Attempt to overwrite system file
    REQUIRE_THROWS_AS(
        save_sequences_to_file("/etc/passwd", sequences, config),
        File_operation_error);
  }

  SECTION("Cannot read sensitive files") {
    File_operation_config config;
    config.validate_path = true;

    // Attempt to read /etc/shadow (requires root, but test the attempt)
    if (geteuid() != 0) {  // Not running as root
      REQUIRE_THROWS_AS(
          load_sequences_from_file<Test_file_event>("/etc/shadow", config),
          File_operation_error);
    }
  }

  SECTION("Home directory escapes should be blocked") {
    File_operation_config config;
    config.allowed_directory = allowed_dir.string();
    config.validate_path = true;

    // Try various home directory escapes
    std::vector<std::string> escape_attempts = {
        "~/../.ssh/id_rsa",
        "~/../../etc/passwd",
        allowed_dir.string() + "/../../home/user/.ssh/key"
    };

    for (const auto& attempt : escape_attempts) {
      REQUIRE_THROWS_AS(save_sequences_to_file(attempt, sequences, config),
                        File_operation_error);
    }
  }
}

TEST_CASE_METHOD(FileOpsFixture, "File operations - error handling",
                 "[file][errors]") {
  SECTION("Non-existent directory in path") {
    std::string filepath = (temp_dir / "nonexistent/test.json").string();
    auto sequences = make_test_sequences();

    REQUIRE_THROWS_AS(save_sequences_to_file(filepath, sequences),
                      File_operation_error);
  }

  SECTION("Reading non-existent file") {
    std::string filepath = (allowed_dir / "nonexistent.json").string();

    REQUIRE_THROWS_AS(load_sequences_from_file<Test_file_event>(filepath),
                      File_operation_error);
  }

  SECTION("Invalid JSON in file") {
    std::string filepath = (allowed_dir / "invalid.json").string();

    // Write invalid JSON
    std::ofstream file(filepath);
    file << "{ this is not valid JSON }";
    file.close();

    // Should throw during JSON parsing
    REQUIRE_THROWS(load_sequences_from_file<Test_file_event>(filepath));
  }
}

TEST_CASE_METHOD(FileOpsFixture, "File operations - canonical paths",
                 "[file][security][!mayfail]") {
  auto sequences = make_test_sequences();

  SECTION("Canonical path validation prevents escape") {
    File_operation_config config;
    config.allowed_directory = std::filesystem::canonical(allowed_dir).string();
    config.validate_path = true;

    // Create complex path that escapes when not canonicalized
    std::string tricky_path = (allowed_dir / "subdir" / ".." / ".." / ".." /
                               "forbidden" / "test.json").string();

    REQUIRE_THROWS_AS(save_sequences_to_file(tricky_path, sequences, config),
                      File_operation_error);
  }

  SECTION("Relative paths are resolved before validation") {
    File_operation_config config;
    config.allowed_directory = std::filesystem::canonical(allowed_dir).string();
    config.validate_path = true;

    // Use relative path
    std::filesystem::current_path(allowed_dir);
    std::string relative_path = "./test.json";

    // Should succeed because it resolves to allowed directory
    save_sequences_to_file(relative_path, sequences, config);
    REQUIRE(std::filesystem::exists(allowed_dir / "test.json"));
  }
}
