#ifndef MICRO_COMPOSER_GUI_SEQUENCER_CONTROLLER_HPP
#define MICRO_COMPOSER_GUI_SEQUENCER_CONTROLLER_HPP

#include "sequencer/atomic_sequencer.h"
#include "sequencable/oscillation_event.h"
#include "synth/synth.h"
#include "synth/synth_output.h"

#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <mutex>

namespace Micro_composer {
namespace gui {

using sequencable::OscillationEvent;
using sequencer::Atomic_sequencer;
using synth::Synthesizer;
using synth::RealTimeAudioOutput;

class SequencerController {
public:
    // Constants
    static constexpr std::size_t DEFAULT_NUM_STEPS = 8;
    static constexpr double DEFAULT_AMPLITUDE = 0.5;
    static constexpr double DEFAULT_PHASE = 0.0;

    SequencerController();
    ~SequencerController();

    // Transport controls
    void start();
    void stop();
    bool is_running() const;

    // Step management
    void set_step_active(std::size_t step_index, bool active);
    bool is_step_active(std::size_t step_index) const;
    std::size_t get_current_step() const;
    bool has_current_step() const;

    void clear_all_steps();
    std::size_t get_num_steps() const { return m_notes.size(); }

    // Step parameter access
    double get_step_frequency(std::size_t step_index) const;
    void set_step_frequency(std::size_t step_index, double frequency);
    double get_step_amplitude(std::size_t step_index) const;
    void set_step_amplitude(std::size_t step_index, double amplitude);
    double get_step_phase(std::size_t step_index) const;
    void set_step_phase(std::size_t step_index, double phase);

    // Status
    bool is_sequencer_initialized() const { return m_sequencer != nullptr; }

private:
    void initialize_sequencer();
    void create_default_sequence();
    std::function<void(OscillationEvent&&)> create_event_handler();

    // Audio components
    std::unique_ptr<RealTimeAudioOutput> m_synth_output;
    std::unique_ptr<Synthesizer> m_synthesizer;
    std::unique_ptr<Atomic_sequencer<OscillationEvent>> m_sequencer;

    // Sequencer data
    std::unique_ptr<atomic_deque::Atomic_deque<OscillationEvent>> m_sequence;

    // Step data
    std::vector<double> m_notes; // Musical notes (frequencies)
    std::array<bool, DEFAULT_NUM_STEPS> m_step_states; // Active/inactive states
    std::array<double, DEFAULT_NUM_STEPS> m_amplitudes; // Step amplitudes
    std::array<double, DEFAULT_NUM_STEPS> m_phases; // Step phases

    // Thread synchronization
    mutable std::mutex m_mutex;

    // Current step tracking
    std::atomic<std::size_t> m_current_step;
    std::atomic<bool> m_has_current;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_SEQUENCER_CONTROLLER_HPP