#ifndef MICRO_COMPOSER_MIDI_OUTPUT_H
#define MICRO_COMPOSER_MIDI_OUTPUT_H

#include <libremidi/libremidi.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Micro_composer {

namespace midi {

// Information about an available MIDI port
struct Port_info {
  int index;
  std::string name;
  std::string display_name; // Formatted for user display
};

// MIDI output wrapper class
class Midi_output {
public:
  // Constructor with optional configuration
  Midi_output();

  // Destructor
  ~Midi_output();

  // List all available MIDI output ports
  static std::vector<Port_info> list_ports();

  // Open a MIDI port by index
  void open_port(int port_index);

  // Open a MIDI port by name
  void open_port(const std::string& port_name);

  // Close the current port
  void close_port();

  // Check if a port is open
  bool is_open() const;

  // Send a MIDI message
  void send_message(const libremidi::message& msg);

  // Get the name of the currently open port
  std::string current_port_name() const;

private:
  std::unique_ptr<libremidi::midi_out> midi_out_;
  int current_port_{-1};
  std::string current_port_name_;
};

} // namespace midi

} // namespace Micro_composer

#endif // MICRO_COMPOSER_MIDI_OUTPUT_H
