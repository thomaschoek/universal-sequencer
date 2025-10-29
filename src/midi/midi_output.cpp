#include "midi/midi_output.h"
#include <stdexcept>
#include <iostream>

namespace Micro_composer {

namespace midi {

Midi_output::Midi_output() {
  // Create MIDI output with default configuration
  midi_out_ = std::make_unique<libremidi::midi_out>();
}

Midi_output::~Midi_output() { close_port(); }

std::vector<Port_info> Midi_output::list_ports() {
  std::vector<Port_info> ports;

  // Create a temporary midi_out to query ports
  libremidi::observer obs{};

  auto output_ports = obs.get_output_ports();

  for (size_t i = 0; i < output_ports.size(); ++i) {
    const auto& port = output_ports[i];
    Port_info info;
    info.index = static_cast<int>(i);
    info.name = port.port_name;
    info.display_name = "[" + std::to_string(i) + "] " + port.port_name;
    ports.push_back(info);
  }

  return ports;
}

void Midi_output::open_port(int port_index) {
  if (is_open()) {
    close_port();
  }

  try {
    libremidi::observer obs{};
    auto output_ports = obs.get_output_ports();

    if (port_index < 0 || port_index >= static_cast<int>(output_ports.size())) {
      throw std::out_of_range("Port index out of range");
    }

    const auto& port = output_ports[port_index];
    midi_out_->open_port(port);
    current_port_ = port_index;
    current_port_name_ = port.port_name;

    std::cout << "[MIDI] Opened port: " << current_port_name_ << std::endl;
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("Failed to open MIDI port: ") +
                             e.what());
  }
}

void Midi_output::open_port(const std::string& port_name) {
  if (is_open()) {
    close_port();
  }

  auto ports = list_ports();
  for (const auto& port : ports) {
    if (port.name == port_name) {
      open_port(port.index);
      return;
    }
  }

  throw std::runtime_error("MIDI port not found: " + port_name);
}

void Midi_output::close_port() {
  if (midi_out_ && is_open()) {
    midi_out_->close_port();
    std::cout << "[MIDI] Closed port: " << current_port_name_ << std::endl;
    current_port_ = -1;
    current_port_name_.clear();
  }
}

bool Midi_output::is_open() const {
  return midi_out_ && midi_out_->is_port_open();
}

void Midi_output::send_message(const libremidi::message& msg) {
  if (!is_open()) {
    throw std::runtime_error("Cannot send MIDI message: no port is open");
  }

  midi_out_->send_message(msg);
}

std::string Midi_output::current_port_name() const { return current_port_name_; }

} // namespace midi

} // namespace Micro_composer
