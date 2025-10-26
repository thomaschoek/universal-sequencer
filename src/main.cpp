#include "sequencable/premade_samples.h"
#include "sequencer/sequencer_template.h"
#include "synth/synth.h"
#include "utility/debug.h"
#include <chrono>
#include <iostream>

int main(int argc, char** argv) {

  using namespace Micro_composer;
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  using Sequencer = Sequencer<Premade_samples>;
  using Sequence = Sequencer::Container;

  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  std::vector<Sequence> sequences;
  {
    Sequence seq1;
    for (size_t note : {39, 51, 39, 43}) {
      auto evt = std::make_unique<Premade_samples>(
          note, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq1.push_back(std::move(evt));
    }

    Sequence seq2;
    for (size_t note : {39, 43, 39, 51, 43, 39, 49, 55}

    ) {
      auto evt = std::make_unique<Premade_samples>(
          note, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq2.push_back(std::move(evt));
    }

    Sequence seq3;
    for (size_t note : {41, 42, 43, 44, 45, 46, 47, 48}) {
      auto evt = std::make_unique<Premade_samples>(
          note, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq3.push_back(std::move(evt));
    }

    Sequence seq4;
    for (size_t note : {27, 29, 31, 33, 35, 37, 39, 41}) {
      auto evt = std::make_unique<Premade_samples>(
          note, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq4.push_back(std::move(evt));
    }

    sequences.push_back(std::move(seq1));
    sequences.push_back(std::move(seq2));
    sequences.push_back(std::move(seq3));
    sequences.push_back(std::move(seq4));
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
        Sequencer::Handler{[&synths, idx](const Premade_samples& event) {
          synths[idx]->write(event.samples_);
        }});
  }

  std::cout << "[MAIN] About to create sequencer\n" << std::flush;

  std::vector<std::unique_ptr<Sequencer>> seqrs;
  for (size_t i = 0; i < SYNTH_VOICES; ++i) {
    seqrs.push_back(
        std::make_unique<Sequencer>(Sequencer{handlers[i], sequences[i]}));
  }

  std::cout << "[MAIN] Sequencer created\n" << std::flush;
  std::cout << "[MAIN] Starting sequencer with repeat=true\n" << std::flush;
  auto start_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);

  std::cout << "[MAIN] About to call start()\n" << std::flush;
  for (const auto& seqr : (seqrs)) {
    seqr->start(start_time, true);
  }
  std::cout << "[MAIN] start() returned\n" << std::flush;

  std::cout << "[MAIN] Playing for 10 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "[MAIN] Pausing sequencer\n";
  for (const auto& seqr : (seqrs)) {
    seqr->pause();
  }

  std::cout << "[MAIN] Waiting 2 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "[MAIN] Changing sequencer tempo\n";
  {
    debug::msg("[MAIN] modifying durations of seqeunce 0");
    size_t i = 0;
    for (auto& evt : sequences[0]) {
      debug::msg("[MAIN] Modifying event " + std::to_string(i++));
      evt->duration = std::chrono::milliseconds(
          static_cast<int>(evt->duration.count() * 0.5)); // Double speed
      debug::msg(
          "[MAIN] Event duration changed to " +
          std::to_string(
              std::chrono::duration_cast<std::chrono::seconds>(evt->duration)
                  .count()) +
          " seconds for event " + std::to_string(i));
      debug::msg("[MAIN] Assigning new samples to event");
      evt->samples_ = Premade_samples::generate_sine_wave(*evt);
    }
    debug::msg("[MAIN] Reassigning sequence data");
    seqrs[0]->assign(sequences[0]);
  }

  std::cout << "[MAIN] Restarting sequencer\n";
  auto restart_time = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  for (const auto& seqr : seqrs) {
    seqr->start(restart_time, true);
  }

  std::cout << "[MAIN] Playing for 5 more seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "[MAIN] Stopping sequencer\n";
  for (const auto& seqr : seqrs) {
    seqr->stop();
  }

  std::cout << "[MAIN] Stopping player thread\n";

  std::cout << "[MAIN] Done!\n";
}
