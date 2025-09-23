#ifndef MICRO_COMPOSER_GUI_APPLICATION_H
#define MICRO_COMPOSER_GUI_APPLICATION_H

#include "sequencable/oscillation_event.h"
#include "sequencer/atomic_sequencer.h"
#include "synth/synth.h"
#include "utils/atomic_deque.h"

#include <memory>
#include <functional>

namespace MicroComposer {
namespace gui {

class GuiApplication {
public:
    using Event_t = sequencable::OscillationEvent;
    using Handler_t = std::function<void(const Event_t&)>;
    using Sequencer_t = sequencer::AtomicSequencer<Event_t, Handler_t>;

    GuiApplication();
    ~GuiApplication();

    int run();
    void shutdown();

private:
    void initialize_sequencer();
    void initialize_default_sequence();

    // GUI event handlers
    void on_play_button_clicked();
    void on_stop_button_clicked();
    void on_tempo_changed(double bpm);
    void on_note_changed(int step, double frequency);

    // Sequencer components
    std::unique_ptr<atomic_deque::AtomicDeque<Event_t>> sequence_;
    std::unique_ptr<synth::RealTimeAudioOutput> audio_output_;
    std::unique_ptr<synth::Synthesizer> synthesizer_;
    std::unique_ptr<Sequencer_t> sequencer_;

    // GUI state
    bool is_playing_;
    double current_bpm_;
};

} // namespace gui
} // namespace MicroComposer

#endif // MICRO_COMPOSER_GUI_APPLICATION_H