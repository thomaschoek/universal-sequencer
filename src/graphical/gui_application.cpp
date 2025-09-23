#include "graphical/gui_application.h"
#include "graphical/main_window.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace MicroComposer {
namespace gui {

GuiApplication::GuiApplication()
    : sequence_(std::make_unique<atomic_deque::AtomicDeque<Event_t>>())
    , audio_output_(std::make_unique<synth::RealTimeAudioOutput>())
    , synthesizer_(std::make_unique<synth::Synthesizer>(*audio_output_))
    , is_playing_(false)
    , current_bpm_(120.0) {

    initialize_default_sequence();
    initialize_sequencer();
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
    // Add a simple C major scale
    sequence_->push_back(Event_t{261.63}); // C4
    sequence_->push_back(Event_t{293.66}); // D4
    sequence_->push_back(Event_t{329.63}); // E4
    sequence_->push_back(Event_t{349.23}); // F4
    sequence_->push_back(Event_t{392.00}); // G4
    sequence_->push_back(Event_t{440.00}); // A4
    sequence_->push_back(Event_t{493.88}); // B4
    sequence_->push_back(Event_t{523.25}); // C5
}

int GuiApplication::run() {
    MainWindow main_window(this);

    // Set up callbacks
    main_window.on_play_clicked = [this]() { on_play_button_clicked(); };
    main_window.on_stop_clicked = [this]() { on_stop_button_clicked(); };
    main_window.on_tempo_changed = [this](double bpm) { on_tempo_changed(bpm); };
    main_window.on_note_changed = [this](int step, double freq) { on_note_changed(step, freq); };

    main_window.show();

    // Main event loop
    while (main_window.process_events()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    return 0;
}

void GuiApplication::shutdown() {
    on_stop_button_clicked();
}

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
    // TODO: Implement tempo change in sequencer
}

void GuiApplication::on_note_changed(int step, double frequency) {
    if (step >= 0 && step < static_cast<int>(sequence_->size())) {
        std::cout << "Step " << step << " frequency changed to: " << frequency << " Hz\n";
        // TODO: Implement live step editing
    }
}

} // namespace gui
} // namespace MicroComposer