#include "graphical/gui_application.h"
#include "graphical/main_window.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace Micro_composer {
namespace gui {

GuiApplication::GuiApplication()
    : sequence_(std::make_unique<atomic_deque::Atomic_deque<Event_t>>()),
      audio_output_(std::make_unique<synth::RealTimeAudioOutput>()),
      synthesizer_(std::make_unique<synth::Synthesizer>(*audio_output_)),
      is_playing_(false), current_bpm_(120.0) {

  initialize_sequencer();
  initialize_default_sequence();
}

GuiApplication::~GuiApplication() {
  if (sequencer_ && sequencer_->is_running()) {
    sequencer_->stop();
  }
}

void GuiApplication::initialize_sequencer() {
  auto handler = [this](const Event_t& event) {
    if (synthesizer_) {
      synthesizer_->play(event);
    }
  };

  sequencer_ = std::make_unique<Sequencer_t>(handler, *sequence_);
}

void GuiApplication::initialize_default_sequence() {
  // Calculate initial timing based on default BPM
  double beat_duration = 60.0 / current_bpm_; // seconds per beat
  std::chrono::duration<double> duration{beat_duration};
  std::chrono::duration<double> offset{0.0};

  // Add a simple C major scale using CRUD interface (now that sequencer_ is initialized)
  sequencer_->push_back(Event_t{261.63, 0.5, 0.0, offset, duration}); // C4
  sequencer_->push_back(Event_t{293.66, 0.5, 0.0, offset, duration}); // D4
  sequencer_->push_back(Event_t{329.63, 0.5, 0.0, offset, duration}); // E4
  sequencer_->push_back(Event_t{349.23, 0.5, 0.0, offset, duration}); // F4
  sequencer_->push_back(Event_t{392.00, 0.5, 0.0, offset, duration}); // G4
  sequencer_->push_back(Event_t{440.00, 0.5, 0.0, offset, duration}); // A4
  sequencer_->push_back(Event_t{493.88, 0.5, 0.0, offset, duration}); // B4
  sequencer_->push_back(Event_t{523.25, 0.5, 0.0, offset, duration}); // C5
}

int GuiApplication::run() {
  MainWindow main_window(this);

  // Set up callbacks
  main_window.on_play_clicked = [this]() { on_play_button_clicked(); };
  main_window.on_stop_clicked = [this]() { on_stop_button_clicked(); };
  main_window.on_tempo_changed = [this](double bpm) { on_tempo_changed(bpm); };
  main_window.on_note_changed = [this](int step, double freq) {
    on_note_changed(step, freq);
  };

  // Set up CRUD operation callbacks
  main_window.on_add_event = [this](double freq, double amp, double phase) {
    add_event(Event_t{freq, amp, phase, std::chrono::duration<double>{0.0}, std::chrono::duration<double>{60.0 / current_bpm_}});
  };
  main_window.on_insert_event = [this](int pos, double freq, double amp, double phase) {
    insert_event_at(pos, Event_t{freq, amp, phase, std::chrono::duration<double>{0.0}, std::chrono::duration<double>{60.0 / current_bpm_}});
  };
  main_window.on_remove_event = [this](int pos) { remove_event_at(pos); };
  main_window.on_remove_events_range = [this](int first, int last) {
    remove_events_range(first, last);
  };

  main_window.show();

  // Main event loop
  while (main_window.process_events()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  return 0;
}

void GuiApplication::shutdown() { on_stop_button_clicked(); }

void GuiApplication::on_play_button_clicked() {
  if (!is_playing_ && sequencer_) {
    sequencer_->start();
    is_playing_ = true;
    std::cout << "Sequencer started\n";
  }
}

void GuiApplication::on_stop_button_clicked() {
  if (is_playing_ && sequencer_) {
    sequencer_->stop();
    is_playing_ = false;
    std::cout << "Sequencer stopped\n";
  }
}

void GuiApplication::on_tempo_changed(double bpm) {
  current_bpm_ = bpm;
  std::cout << "Tempo changed to: " << bpm << " BPM\n";

  // Calculate new duration based on BPM (quarter note duration)
  double beat_duration = 60.0 / bpm; // seconds per beat
  std::chrono::duration<double> new_duration{beat_duration};

  // Update all events in the sequence with new duration using CRUD interface
  try {
    size_t sequence_size = get_sequence_size();
    for (size_t i = 0; i < sequence_size; ++i) {
      Event_t event = get_event_at(i);
      event.duration = new_duration;
      update_event_at(i, std::move(event));
    }
    std::cout << "Updated event durations to " << beat_duration
              << " seconds per beat\n";
  } catch (const std::exception& e) {
    std::cerr << "Error updating tempo: " << e.what() << std::endl;
  }
}

void GuiApplication::on_note_changed(int step, double frequency) {
  try {
    size_t sequence_size = get_sequence_size();
    if (step >= 0 && step < static_cast<int>(sequence_size)) {
      std::cout << "Step " << step << " frequency changed to: " << frequency
                << " Hz\n";

      // Update the frequency of the specified step using CRUD interface
      Event_t event = get_event_at(step);
      event.frequency = frequency;
      update_event_at(step, std::move(event));
      std::cout << "Successfully updated step " << step << " frequency\n";
    } else {
      std::cout << "Invalid step number: " << step << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error updating note: " << e.what() << std::endl;
  }
}

// CRUD operations for sequence management
void GuiApplication::add_event(Event_t&& event) {
  try {
    sequencer_->push_back(std::move(event));
    std::cout << "Event added to sequence" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error adding event: " << e.what() << std::endl;
  }
}

void GuiApplication::insert_event_at(size_t position, Event_t&& event) {
  try {
    // For now, just use push_back - proper insert with iterator would need more work
    sequencer_->push_back(std::move(event));
    std::cout << "Event added to end of sequence (insert at position not yet implemented)" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error inserting event: " << e.what() << std::endl;
  }
}

void GuiApplication::update_event_at(size_t position, Event_t&& event) {
  try {
    // For now, use the old approach since the CRUD interface needs iterators
    if (position < get_sequence_size()) {
      std::scoped_lock lock(sequence_->lock());
      auto it = sequence_->begin() + position;
      if (it != sequence_->end()) {
        *it = std::move(event);
        std::cout << "Event at position " << position << " updated" << std::endl;
      }
    } else {
      std::cout << "Invalid position: " << position << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error updating event: " << e.what() << std::endl;
  }
}

void GuiApplication::remove_event_at(size_t position) {
  try {
    if (position == 0) {
      sequencer_->pop_front();
      std::cout << "First event removed" << std::endl;
    } else if (position == get_sequence_size() - 1) {
      sequencer_->pop_back();
      std::cout << "Last event removed" << std::endl;
    } else {
      std::cout << "Remove at arbitrary position not yet implemented" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error removing event: " << e.what() << std::endl;
  }
}

void GuiApplication::remove_events_range(size_t first, size_t last) {
  try {
    // For now, use multiple pop operations for simplicity
    if (first == 0) {
      for (size_t i = first; i < last && i < get_sequence_size(); ++i) {
        sequencer_->pop_front();
      }
      std::cout << "Events from front removed" << std::endl;
    } else {
      std::cout << "Remove range at arbitrary positions not yet implemented" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error removing events: " << e.what() << std::endl;
  }
}

GuiApplication::Event_t GuiApplication::get_event_at(size_t position) const {
  try {
    if (position < get_sequence_size()) {
      std::scoped_lock lock(sequence_->lock());
      auto it = sequence_->begin() + position;
      if (it != sequence_->end()) {
        return *it;
      }
    }
    throw std::out_of_range("Invalid position: " + std::to_string(position));
  } catch (const std::exception& e) {
    std::cerr << "Error getting event: " << e.what() << std::endl;
    throw;
  }
}

size_t GuiApplication::get_sequence_size() const {
  try {
    return sequence_->size();
  } catch (const std::exception& e) {
    std::cerr << "Error getting sequence size: " << e.what() << std::endl;
    return 0;
  }
}

} // namespace gui
} // namespace Micro_composer