#include "sequencable/oscillation_event.h"
#include "sequencer/sequencer.h"
#include "synth/synth.h"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {

  using namespace Micro_composer;
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  std::vector<Oscillation_event> seq1;
  std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23,
                                      392.00, 440.00, 493.88, 523.25};
  for (auto freq : frequencies1) {
    Oscillation_event evt;
    evt.frequency = freq;
    evt.duration = std::chrono::milliseconds{250};
    seq1.push_back(evt);
  }

  std::vector<Oscillation_event> seq2;
  std::vector<double> frequencies2 = {523.25, 493.88, 440.00, 392.00,
                                      349.23, 329.63, 293.66, 261.63};
  for (auto freq : frequencies2) {
    Oscillation_event evt;
    evt.frequency = freq;
    evt.duration = std::chrono::milliseconds{250};
    seq1.push_back(evt);
  }

  std::vector<std::vector<Oscillation_event>> sequences = {seq1, seq2};

  // Set up audio output - create a pool of synthesizers
  constexpr std::size_t SYNTH_POOL_SIZE = 10;
  std::vector<std::unique_ptr<RealTimeAudioOutput>> synth_outputs;
  std::vector<std::unique_ptr<Synthesizer>> synths;

  for (std::size_t i = 0; i < SYNTH_POOL_SIZE; ++i) {
    synth_outputs.push_back(std::make_unique<RealTimeAudioOutput>());
    synths.push_back(std::make_unique<Synthesizer>(*synth_outputs.back()));
  }

  auto sequencer = Sequencer<Oscillation_event>(sequences[0]);
  sequencer.start();
  std::this_thread::sleep_for(std::chrono::seconds(10));
  sequencer.pause();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  sequencer.start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  sequencer.reset();
}
