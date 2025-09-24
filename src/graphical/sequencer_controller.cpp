#include "graphical/sequencer_controller.hpp"
#include "sequencer/atomic_sequencer.tpp"
#include "synth/synth.tpp"
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>

namespace Micro_composer {
namespace gui {

SequencerController::SequencerController()
    : m_current_step(0), m_has_current(false) {
    // Initialize default musical notes (C major scale)
    m_notes = {
        261.63, // C4
        293.66, // D4
        329.63, // E4
        349.23, // F4
        392.00, // G4
        440.00, // A4
        493.88, // B4
        523.25  // C5
    };

    // Initialize step states (all inactive by default)
    m_step_states.fill(false);

    initialize_sequencer();
}

SequencerController::~SequencerController() {
    if (m_sequencer && m_sequencer->is_running()) {
        stop();
    }
}

void SequencerController::initialize_sequencer() {
    try {
        m_synth_output = std::make_unique<RealTimeAudioOutput>();
        m_synthesizer = std::make_unique<Synthesizer>(*m_synth_output);

        // Create the sequence first
        m_sequence = std::make_unique<atomic_deque::Atomic_deque<OscillationEvent>>();

        auto handler = create_event_handler();
        m_sequencer = std::make_unique<Atomic_sequencer<OscillationEvent>>(handler, *m_sequence);

        create_default_sequence();
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize sequencer: " << e.what() << std::endl;
        throw;
    }
}

std::function<void(OscillationEvent&&)> SequencerController::create_event_handler() {
    return [this](OscillationEvent&& event) {
        // Update current step tracking
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find which step this event corresponds to
        for (std::size_t i = 0; i < m_notes.size(); ++i) {
            if (std::abs(event.frequency - m_notes[i]) < 0.01) {
                m_current_step.store(i);
                m_has_current.store(true);
                break;
            }
        }

        // Play the event if synthesizer is available
        if (m_synthesizer) {
            try {
                m_synthesizer->play(event);
            } catch (const std::exception& e) {
                std::cerr << "Error playing event: " << e.what() << std::endl;
            }
        }
    };
}

void SequencerController::create_default_sequence() {
    if (!m_sequence) {
        throw std::runtime_error("Sequence not initialized");
    }

    // Clear existing sequence
    while (!m_sequence->empty()) {
        m_sequence->pop_back();
    }

    // Add active steps to the sequence
    for (std::size_t i = 0; i < m_step_states.size(); ++i) {
        if (m_step_states[i]) {
            OscillationEvent event(m_notes[i], DEFAULT_AMPLITUDE, DEFAULT_PHASE);
            m_sequence->push_back(std::move(event));
        }
    }
}

void SequencerController::start() {
    if (!m_sequencer) {
        throw std::runtime_error("Sequencer not initialized");
    }

    if (m_sequencer->is_running()) {
        return;
    }

    // Recreate sequence with current step states
    create_default_sequence();

    m_sequencer->start();
}

void SequencerController::stop() {
    if (m_sequencer && m_sequencer->is_running()) {
        m_sequencer->stop();
        m_has_current.store(false);
    }
}

bool SequencerController::is_running() const {
    return m_sequencer && m_sequencer->is_running();
}

void SequencerController::set_step_active(std::size_t step_index, bool active) {
    if (step_index >= m_step_states.size()) {
        throw std::out_of_range("Step index out of range");
    }

    bool was_running = false;

    // Update the step state first
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_step_states[step_index] = active;
        was_running = is_running();
    }

    // If sequencer was running, restart it outside of the lock to avoid deadlock
    if (was_running) {
        stop();
        // Give a small delay to ensure stop completes
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        start();
    }
}

bool SequencerController::is_step_active(std::size_t step_index) const {
    if (step_index >= m_step_states.size()) {
        throw std::out_of_range("Step index out of range");
    }
    return m_step_states[step_index];
}

std::size_t SequencerController::get_current_step() const {
    return m_current_step.load();
}

bool SequencerController::has_current_step() const {
    return m_has_current.load();
}

void SequencerController::clear_all_steps() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_step_states.fill(false);

    if (is_running()) {
        stop();
    }
}

} // namespace gui
} // namespace Micro_composer