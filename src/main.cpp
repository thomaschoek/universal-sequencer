#include "sequencable/oscillation_event.h"
#include "sequencer/sequencer_template.h"
#include "synth/synth.h"
#include <chrono>
#include <future>
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
  std::vector<Sequence> sequences;
  {
    Sequence seq1;
    std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23};
    //, 392.00, 440.00, 493.88, 523.25};
    for (auto freq : frequencies1) {
      auto evt = std::make_unique<Oscillation_event>();
      evt->frequency = freq;
      evt->duration = std::chrono::milliseconds{250};
      seq1.push_back(std::move(evt));
    }

    Sequence seq2;
    std::vector<double> frequencies2 = {523.25, 493.88, 440.00, 392.00,
                                        349.23, 329.63, 293.66, 261.63};
    for (auto freq : frequencies2) {
      auto evt = std::make_unique<Oscillation_event>();
      evt->frequency = freq;
      evt->duration = std::chrono::milliseconds{250};
      seq2.push_back(std::move(evt));
    }

    seq1[2]->duration = std::chrono::milliseconds{500};
    seq2[2]->duration = std::chrono::milliseconds{500};
    seq2[6]->duration = std::chrono::milliseconds{500};
    sequences.push_back(std::move(seq1));
    sequences.push_back(std::move(seq2));
  }

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

  Sequencer::Handler proxy_handler{[&handlers](Oscillation_event&& evt) {
    static std::future<void> calls[2] = {
        std::async(std::launch::async, handlers[0], std::move(evt)),
        std::async(std::launch::async, handlers[1], std::move(evt))};
    calls[0].get();
    calls[1].get();
  }};

  std::cout << "[MAIN] About to create sequencer\n" << std::flush;

  auto seqr1 = Sequencer(handlers[0], sequences[0]);
  auto seqr2 = Sequencer(handlers[1], sequences[1]);

  std::cout << "[MAIN] Sequencer created\n" << std::flush;
  std::cout << "[MAIN] Starting sequencer with repeat=true\n" << std::flush;
  auto start_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);

  std::cout << "[MAIN] About to call start()\n" << std::flush;
  seqr1.start(start_time, true);
  seqr2.start(start_time, true);
  std::cout << "[MAIN] start() returned\n" << std::flush;

  std::cout << "[MAIN] Playing for 10 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "[MAIN] Pausing sequencer\n";
  seqr1.pause();

  std::cout << "[MAIN] Waiting 2 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));
  seqr2.pause();

  std::cout << "[MAIN] Changing sequencer tempo\n";
  {
    for (auto& evt : sequences[0]) {
      evt->duration = std::chrono::milliseconds(
          static_cast<int>(evt->duration.count() * 0.5)); // Double speed
    }
    seqr1.assign(sequences[0]);
  }

  std::cout << "[MAIN] Restarting sequencer\n";
  auto restart_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  seqr1.start(restart_time, true);

  std::cout << "[MAIN] Playing for 5 more seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "[MAIN] Stopping sequencer\n";
  seqr1.stop();

  std::cout << "[MAIN] Stopping player thread\n";

  std::cout << "[MAIN] Done!\n";
}
