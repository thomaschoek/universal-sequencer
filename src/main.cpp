#include "sequencable/premade_samples.h"
#include "sequencer/poly_sequencer_template.h"
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
  using Poly_seq = Poly_sequencer<Premade_samples>;

  // Create example sequences
  std::vector<std::vector<Premade_samples>> sequences;
  {
    std::vector<Premade_samples> seq1;
    for (size_t note : {39, 51, 39, 43}) {
      seq1.emplace_back(note, 0.5, 0.0, std::chrono::milliseconds{0},
                        std::chrono::milliseconds{100});
    }

    std::vector<Premade_samples> seq2;
    for (size_t note : {39, 43, 39, 51, 43, 39, 49, 55}) {
      seq2.emplace_back(note, 0.5, 0.0, std::chrono::milliseconds{0},
                        std::chrono::milliseconds{100});
    }

    std::vector<Premade_samples> seq3;
    for (size_t note : {41, 42, 43, 44, 45, 46, 47, 48}) {
      seq3.emplace_back(note, 0.5, 0.0, std::chrono::milliseconds{0},
                        std::chrono::milliseconds{100});
    }

    std::vector<Premade_samples> seq4;
    for (size_t note : {27, 29, 31, 33, 35, 37, 39, 41}) {
      seq4.emplace_back(note, 0.5, 0.0, std::chrono::milliseconds{0},
                        std::chrono::milliseconds{100});
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
  std::vector<Poly_seq::Handler> handlers;
  for (std::size_t i = 0; i < SYNTH_VOICES; ++i) {
    synth_outputs.emplace_back(std::make_unique<RealTimeAudioOutput>());
    synths.emplace_back(std::make_unique<Synthesizer>(*synth_outputs.back()));
    const auto idx = i;
    handlers.emplace_back(
        Poly_seq::Handler{[&synths, idx](Premade_samples&& event) {
          synths[idx]->write(event.samples_);
        }});
  }

  std::cout << "[MAIN] About to create poly sequencer\n" << std::flush;

  Poly_seq poly_seq(handlers, std::move(sequences));

  std::cout << "[MAIN] Poly sequencer initialized\n" << std::flush;
  std::cout << "[MAIN] Starting sequencers with repeat=true\n" << std::flush;
  auto t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);

  poly_seq.start_all(t_point, true);
  std::cout << "[MAIN] Playing for 2 seconds with all events disabled\n"
            << std::flush;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  poly_seq.enable();

  std::cout << "[MAIN] Playing for 10 seconds with all events enabled...\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "[MAIN] Pausing sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.pause_all(t_point);

  std::cout << "[MAIN] Waiting 2 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "[MAIN] Changing sequencer tempos\n";
  poly_seq.multiply_durations_all(4); // Slower

  std::cout << "[MAIN] Restarting sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.start_all(t_point, true);

  std::cout << "[MAIN] Playing for 5 more seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "[MAIN] Pausing sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.pause_all(t_point);

  std::cout << "[MAIN] Adjusting tempos again\n";
  poly_seq.adjust_durations_all(std::chrono::milliseconds{-100}); // Faster

  std::cout << "[MAIN] Restarting sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.start_all(t_point, true);

  std::cout << "[MAIN] Playing for 3 moar seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(4));

  std::cout << "[MAIN] Adjusting tempo again...\n";
  poly_seq.adjust_durations_all(std::chrono::milliseconds{-50}); // Even faster

  std::cout << "[MAIN] Restarting sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.start_all(t_point, true);

  std::this_thread::sleep_for(std::chrono::seconds(4));

  std::cout << "[MAIN] Adjusting tempo again...\n";
  poly_seq.multiply_durations_all(0.5); // Even faster

  std::this_thread::sleep_for(std::chrono::seconds(4));

  std::cout << "[MAIN] Stopping sequencers\n";
  t_point = Sequencer::Clock::now() + std::chrono::milliseconds(50);
  poly_seq.stop_all(t_point);

  std::cout << "[MAIN] Done!\n";
}
