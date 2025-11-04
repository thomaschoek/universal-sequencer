#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "gui/json_serialization.h"
#include "gui/event_parameter_traits.h"
#include <chrono>
#include <string>

using namespace Micro_composer::gui;
using namespace Micro_composer::sequencable;

// Simple test event for JSON serialization testing
struct Test_json_event {
  bool enabled{true};
  Duration duration{std::chrono::milliseconds(100)};
  Time_point scheduled_time{};
  double frequency{440.0};
  std::string name{"default"};

  void set_duration(Duration d) { duration = d; }
  void set_scheduled_time(Time_point t) { scheduled_time = t; }
  void update(const Test_json_event& other) {
    enabled = other.enabled;
    duration = other.duration;
    scheduled_time = other.scheduled_time;
    frequency = other.frequency;
    name = other.name;
  }
};

// Specialize Event_parameter_traits for Test_json_event
template <>
struct Micro_composer::gui::Event_parameter_traits<Test_json_event> {
  static constexpr size_t parameter_count = 4;

  static std::string get_parameter_name(size_t index) {
    switch (index) {
    case 0:
      return "enabled";
    case 1:
      return "duration_ms";
    case 2:
      return "frequency";
    case 3:
      return "name";
    default:
      throw std::out_of_range("Invalid parameter index");
    }
  }

  static std::string get_parameter_value(const Test_json_event& event,
                                          size_t index) {
    switch (index) {
    case 0:
      return event.enabled ? "true" : "false";
    case 1:
      return std::to_string(
          std::chrono::duration_cast<std::chrono::milliseconds>(event.duration)
              .count());
    case 2:
      return std::to_string(event.frequency);
    case 3:
      return event.name;
    default:
      throw std::out_of_range("Invalid parameter index");
    }
  }

  static void set_parameter_value(Test_json_event& event, size_t index,
                                   const std::string& value) {
    switch (index) {
    case 0:
      event.enabled = (value == "true" || value == "1");
      break;
    case 1:
      event.duration = std::chrono::milliseconds(std::stoi(value));
      break;
    case 2:
      event.frequency = std::stod(value);
      break;
    case 3:
      event.name = value;
      break;
    default:
      throw std::out_of_range("Invalid parameter index");
    }
  }
};

TEST_CASE("JSON serialization - valid sequences", "[json][serialization]") {
  SECTION("Empty sequences") {
    std::vector<std::vector<Test_json_event>> sequences;
    std::string json = sequences_to_json(sequences);

    REQUIRE_FALSE(json.empty());
    REQUIRE(json.find("\"sequencers\":") != std::string::npos);

    auto parsed = json_to_sequences<Test_json_event>(json);
    REQUIRE(parsed.empty());
  }

  SECTION("Single sequencer with one event") {
    Test_json_event event;
    event.enabled = true;
    event.duration = std::chrono::milliseconds(200);
    event.frequency = 880.0;
    event.name = "test_event";

    std::vector<std::vector<Test_json_event>> sequences = {{event}};
    std::string json = sequences_to_json(sequences);

    auto parsed = json_to_sequences<Test_json_event>(json);
    REQUIRE(parsed.size() == 1);
    REQUIRE(parsed[0].size() == 1);
    REQUIRE(parsed[0][0].enabled == true);
    REQUIRE(parsed[0][0].duration == std::chrono::milliseconds(200));
    REQUIRE(parsed[0][0].frequency == 880.0);
    REQUIRE(parsed[0][0].name == "test_event");
  }

  SECTION("Multiple sequencers with multiple events") {
    Test_json_event event1;
    event1.name = "event1";
    event1.frequency = 440.0;

    Test_json_event event2;
    event2.name = "event2";
    event2.frequency = 880.0;

    std::vector<std::vector<Test_json_event>> sequences = {
        {event1, event2}, {event1}, {event2, event1, event2}};

    std::string json = sequences_to_json(sequences);
    auto parsed = json_to_sequences<Test_json_event>(json);

    REQUIRE(parsed.size() == 3);
    REQUIRE(parsed[0].size() == 2);
    REQUIRE(parsed[1].size() == 1);
    REQUIRE(parsed[2].size() == 3);
    REQUIRE(parsed[0][0].name == "event1");
    REQUIRE(parsed[0][1].name == "event2");
  }
}

TEST_CASE("JSON parsing - security: DoS protection",
          "[json][security][!mayfail]") {
  SECTION("Deeply nested JSON should be rejected") {
    // Create JSON with extreme nesting depth (>100 levels)
    std::string deeply_nested = "{\"sequencers\": [";
    for (int i = 0; i < 150; ++i) {
      deeply_nested += "{\"nested" + std::to_string(i) + "\": [";
    }
    deeply_nested += "]}";
    for (int i = 0; i < 150; ++i) {
      deeply_nested += "]}";
    }

    Json_parse_config config;
    config.max_nesting_depth = 100;

    // Should throw Json_parse_error, not crash or hang
    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(deeply_nested, config),
                      Json_parse_error);
  }

  SECTION("Extremely large file should be rejected") {
    Json_parse_config config;
    config.max_file_size = 1024; // 1 KB limit for test

    // Create JSON larger than limit
    std::string large_json = "{\"sequencers\": [{\"events\": [";
    while (large_json.size() < 2048) {
      large_json += "{\"enabled\": \"true\", \"duration_ms\": \"100\", "
                    "\"frequency\": \"440.0\", \"name\": \"test\"},";
    }
    large_json += "]}]}";

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(large_json, config),
                      Json_parse_error);
  }

  SECTION("Extremely long string should be rejected") {
    Json_parse_config config;
    config.max_string_length = 1000;

    // Create event with very long name
    std::string long_name(5000, 'a'); // 5000 characters
    Test_json_event event;
    event.name = long_name;

    std::vector<std::vector<Test_json_event>> sequences = {{event}};
    std::string json = sequences_to_json(sequences);

    // Should reject strings exceeding max length with clear error
    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(json, config),
                      Json_parse_error);
  }
}

TEST_CASE("JSON parsing - security: malformed JSON", "[json][security][!mayfail]") {
  SECTION("Missing closing brace") {
    std::string malformed = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": "true",
              "duration_ms": "100"
    )"; // Missing closing braces

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(malformed),
                      Json_parse_error);
  }

  SECTION("Invalid JSON structure") {
    std::string invalid = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": true,  // Missing quotes around value
              "duration_ms": 100,
            }
          ]
        }
      ]
    })";

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(invalid),
                      Json_parse_error);
  }

  SECTION("Extra commas") {
    std::string extra_commas = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": "true",,
              "duration_ms": "100",
            }
          ]
        },
      ]
    })";

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(extra_commas),
                      Json_parse_error);
  }

  SECTION("Unescaped quotes in strings") {
    std::string unescaped = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": "true",
              "name": "test"name"
            }
          ]
        }
      ]
    })";

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(unescaped),
                      Json_parse_error);
  }
}

TEST_CASE("JSON parsing - validation", "[json][validation][!mayfail]") {
  SECTION("Invalid parameter values should be rejected") {
    std::string invalid_value = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": "true",
              "duration_ms": "not_a_number",
              "frequency": "440.0",
              "name": "test"
            }
          ]
        }
      ]
    })";

    REQUIRE_THROWS_AS(json_to_sequences<Test_json_event>(invalid_value),
                      Json_parse_error);
  }

  SECTION("Unknown parameters should be ignored or rejected") {
    std::string unknown_param = R"({
      "sequencers": [
        {
          "events": [
            {
              "enabled": "true",
              "duration_ms": "100",
              "unknown_field": "value",
              "frequency": "440.0",
              "name": "test"
            }
          ]
        }
      ]
    })";

    // Should either ignore unknown fields or reject with clear error
    // Current implementation likely ignores, which may be okay
    auto parsed = json_to_sequences<Test_json_event>(unknown_param);
    REQUIRE(parsed.size() == 1);
  }
}

TEST_CASE("JSON parsing - special characters", "[json][encoding][!mayfail]") {
  SECTION("Unicode characters in strings") {
    Test_json_event event;
    event.name = "test_\u00E9\u00F1\u4E2D";  // é, ñ, 中

    std::vector<std::vector<Test_json_event>> sequences = {{event}};
    std::string json = sequences_to_json(sequences);

    auto parsed = json_to_sequences<Test_json_event>(json);
    REQUIRE(parsed[0][0].name == event.name);
  }

  SECTION("Escaped characters in strings") {
    Test_json_event event;
    event.name = R"(test "quotes" and \backslashes\)";

    std::vector<std::vector<Test_json_event>> sequences = {{event}};
    std::string json = sequences_to_json(sequences);

    auto parsed = json_to_sequences<Test_json_event>(json);
    REQUIRE(parsed[0][0].name == event.name);
  }

  SECTION("Newlines and tabs in strings") {
    Test_json_event event;
    event.name = "line1\nline2\ttabbed";

    std::vector<std::vector<Test_json_event>> sequences = {{event}};
    std::string json = sequences_to_json(sequences);

    auto parsed = json_to_sequences<Test_json_event>(json);
    REQUIRE(parsed[0][0].name == event.name);
  }
}

TEST_CASE("JSON round-trip consistency", "[json][roundtrip]") {
  SECTION("Complex sequence maintains integrity through round-trip") {
    std::vector<std::vector<Test_json_event>> original;

    for (int seq = 0; seq < 3; ++seq) {
      std::vector<Test_json_event> events;
      for (int evt = 0; evt < 5; ++evt) {
        Test_json_event e;
        e.enabled = (evt % 2 == 0);
        e.duration = std::chrono::milliseconds(100 * (evt + 1));
        e.frequency = 440.0 * (1 << evt);
        e.name = "seq" + std::to_string(seq) + "_evt" + std::to_string(evt);
        events.push_back(e);
      }
      original.push_back(events);
    }

    std::string json = sequences_to_json(original);
    auto parsed = json_to_sequences<Test_json_event>(json);

    REQUIRE(parsed.size() == original.size());
    for (size_t i = 0; i < original.size(); ++i) {
      REQUIRE(parsed[i].size() == original[i].size());
      for (size_t j = 0; j < original[i].size(); ++j) {
        REQUIRE(parsed[i][j].enabled == original[i][j].enabled);
        REQUIRE(parsed[i][j].duration == original[i][j].duration);
        REQUIRE(parsed[i][j].frequency == original[i][j].frequency);
        REQUIRE(parsed[i][j].name == original[i][j].name);
      }
    }
  }
}
