#ifndef MICRO_COMPOSER_GUI_APPLICATION_H
#define MICRO_COMPOSER_GUI_APPLICATION_H

#include "sequencable/oscillation_event.h"
#include "sequencer/atomic_sequencer.tpp"
#include "synth/synth.tpp"

#include <functional>
#include <memory>

namespace Micro_composer {
namespace gui {

class GuiApplication {
public:
  using Event_t = sequencable::OscillationEvent;
  using Handler_t = std::function<void(const Event_t&)>;
  using Sequencer_t = sequencer::Atomic_sequencer<Event_t>;

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

  // CRUD operations for sequence management
  void add_event(Event_t&& event);
  void insert_event_at(size_t position, Event_t&& event);
  void update_event_at(size_t position, Event_t&& event);
  void remove_event_at(size_t position);
  void remove_events_range(size_t first, size_t last);
  Event_t get_event_at(size_t position) const;
  size_t get_sequence_size() const;

  // Sequencer components
  std::unique_ptr<atomic_deque::Atomic_deque<Event_t>> sequence_;
  std::unique_ptr<synth::RealTimeAudioOutput> audio_output_;
  std::unique_ptr<synth::Synthesizer> synthesizer_;
  std::unique_ptr<Sequencer_t> sequencer_;

  // GUI state
  bool is_playing_;
  double current_bpm_;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_APPLICATION_H