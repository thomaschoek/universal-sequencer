#include "sequencable/oscillation_event.h"
#include "sequencer/sequencer_template.h"
#include "synth/synth.h"
#include <chrono>
#include <iostream>

int main(int argc, char** argv) {

  using namespace Micro_composer;
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  using Sequencer = Sequencer<Oscillation_event>;
  using Sequence = Sequencer::Container;

  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  Sequence seq1;
  std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23};
  //, 392.00, 440.00, 493.88, 523.25};
  for (auto freq : frequencies1) {
    Oscillation_event* evt = new Oscillation_event;
    evt->frequency = freq;
    evt->duration = std::chrono::milliseconds{250};
    seq1.push_back(evt);
  }

  Sequence seq2;
  std::vector<double> frequencies2 = {523.25, 493.88, 440.00, 392.00,
                                      349.23, 329.63, 293.66, 261.63};
  for (auto freq : frequencies2) {
    Oscillation_event* evt = new Oscillation_event;
    evt->frequency = freq;
    evt->duration = std::chrono::milliseconds{250};
    seq2.push_back(evt);
  }

  std::vector<Sequence> sequences = {seq1, seq2};

  // Set up audio output - create a pool of synthesizers
  constexpr std::size_t SYNTH_VOICES = 4;
  std::vector<std::unique_ptr<RealTimeAudioOutput>> synth_outputs;
  std::vector<std::unique_ptr<Synthesizer>> synths;
  std::vector<Sequencer::Handler> handlers;
  for (std::size_t i = 0; i < SYNTH_VOICES; ++i) {
    synth_outputs.emplace_back(std::make_unique<RealTimeAudioOutput>());
    synths.emplace_back(std::make_unique<Synthesizer>(*synth_outputs.back()));
    const auto idx = i;
    handlers.emplace_back(
        Sequencer::Handler{[&synths, idx](const Oscillation_event& event) {
          synths[idx]->play(event);
        }});
  }

  auto sequencer = Sequencer(sequences[0]);

  std::cout << "[MAIN] Starting sequencer with repeat=true\n";
  auto start_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);

  sequencer.start(start_time, true);

  // Create a consumer thread that plays events from the sequencer
  std::jthread subscription = sequencer.subscribe(handlers[0]);

  std::cout << "[MAIN] Playing for 10 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "[MAIN] Pausing sequencer\n";
  sequencer.pause();

  subscription = sequencer.subscribe(handlers[0]);
  std::jthread sub2 = sequencer.subscribe(handlers[1]);

  std::cout << "[MAIN] Waiting 2 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "[MAIN] Restarting sequencer\n";
  auto restart_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  sequencer.start(restart_time, true);

  std::cout << "[MAIN] Playing for 5 more seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "[MAIN] Stopping sequencer\n";
  sequencer.stop();

  std::cout << "[MAIN] Stopping player thread\n";
  subscription.request_stop();
  sub2.request_stop();

  std::cout << "[MAIN] Done!\n";
}
