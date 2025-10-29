#include "sequencable/midi_event.h"
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace Micro_composer {

namespace sequencable {

// Constructor from note name
Midi_event::Midi_event(Duration dur, const std::string& note_name, uint8_t vel)
    : Mutable_event{dur}, channel(0), note(name_to_note(note_name)),
      velocity(vel) {}

// Generate MIDI note-on and note-off messages
Midi_event::Messages Midi_event::to_midi_messages() const {
  Messages msgs;

  // Note-on message: status byte (0x90 + channel), note, velocity
  msgs.note_on.bytes = {
      static_cast<uint8_t>(0x90 | (channel & 0x0F)),
      static_cast<uint8_t>(note & 0x7F),
      static_cast<uint8_t>(velocity & 0x7F)};

  // Note-off message: status byte (0x80 + channel), note, velocity 0
  msgs.note_off.bytes = {
      static_cast<uint8_t>(0x80 | (channel & 0x0F)),
      static_cast<uint8_t>(note & 0x7F), 0};

  return msgs;
}

// Convert note number to name
std::string Midi_event::note_to_name(uint8_t note_num) {
  if (note_num > 127) {
    throw std::invalid_argument("Note number must be 0-127");
  }

  const char* note_names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                              "F#", "G",  "G#", "A",  "A#", "B"};

  int octave = (note_num / 12) - 1; // MIDI octave -1 to 9
  int pitch_class = note_num % 12;

  return std::string(note_names[pitch_class]) + std::to_string(octave);
}

// Convert note name to number
uint8_t Midi_event::name_to_note(const std::string& name) {
  if (name.empty()) {
    throw std::invalid_argument("Note name cannot be empty");
  }

  // Parse note name (e.g., "C4", "D#5", "Bb3")
  std::string note_str = name;

  // Extract pitch class (C, D, E, etc.)
  char pitch = std::toupper(note_str[0]);
  if (pitch < 'A' || pitch > 'G') {
    throw std::invalid_argument("Invalid note name: must start with A-G");
  }

  // Map pitch to semitone (C=0, D=2, E=4, F=5, G=7, A=9, B=11)
  const int pitch_values[] = {9, 11, 0, 2, 4, 5, 7}; // A B C D E F G
  int semitone = pitch_values[pitch - 'A'];

  size_t idx = 1;

  // Check for sharp or flat
  if (idx < note_str.length()) {
    if (note_str[idx] == '#') {
      semitone++;
      idx++;
    } else if (note_str[idx] == 'b') {
      semitone--;
      idx++;
    }
  }

  // Extract octave number
  if (idx >= note_str.length()) {
    throw std::invalid_argument("Note name must include octave number");
  }

  int octave = 0;
  try {
    octave = std::stoi(note_str.substr(idx));
  } catch (...) {
    throw std::invalid_argument("Invalid octave number in note name");
  }

  // Calculate MIDI note number
  int note_num = (octave + 1) * 12 + semitone;

  if (note_num < 0 || note_num > 127) {
    throw std::out_of_range("Note number out of MIDI range (0-127)");
  }

  return static_cast<uint8_t>(note_num);
}

} // namespace sequencable

} // namespace Micro_composer
