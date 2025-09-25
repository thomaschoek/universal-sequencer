#include "sequencable/oscillation_event.h"
#include "sequencer/multi_sequencer.tpp"
#include "synth/synth.tpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::synth;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::sequence;
  Atomic_step_sequence<OscillationEvent> steps;

  // Add some steps with different frequencies (musical notes)
  // Each step: offset, duration, {frequency, amplitude, phase}
  steps.push_back(OscillationEvent{261.63}); // C4
  steps.push_back(OscillationEvent{293.66}); // D4
  steps.push_back(OscillationEvent{329.63}); // E4
  steps.push_back(OscillationEvent{349.23}); // F4
  steps.push_back(OscillationEvent{392.00}); // G4
  steps.push_back(OscillationEvent{440.00}); // A4
  steps.push_back(OscillationEvent{493.88}); // B4
  steps.push_back(OscillationEvent{523.25}); // C5

  Atomic_step_sequence<OscillationEvent> reverse_steps;
  for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
    reverse_steps.push_back(*it);
  }

  RealTimeAudioOutput synth_out_1, synth_out_2;
  Synthesizer synth_1{synth_out_1}, synth_2{synth_out_2};
  std::function<void(OscillationEvent&&)> handler_1 =
      [&synth_1](const OscillationEvent&& event) {
        std::ignore = std::async(std::launch::async,
                                 [&synth_1, event]() { synth_1.play(event); });
      };
  decltype(handler_1) handler_2 = [&synth_2](const OscillationEvent&& event) {
    std::ignore = std::async(std::launch::async,
                             [&synth_2, event]() { synth_2.play(event); });
  };

  std::vector<Micro_composer::sequence::Atomic_step_sequence<OscillationEvent>>
      sequences;
  sequences.emplace_back(steps);
  sequences.emplace_back(reverse_steps);

  std::vector<decltype(handler_1)> handlers;
  handlers.emplace_back(handler_1);
  handlers.emplace_back(handler_2);

  Multi_sequencer<OscillationEvent> seqr{handlers, sequences};

  auto t_start = std::chrono::steady_clock::now();
  seqr.start();

  std::this_thread::sleep_for(std::chrono::seconds(30));
  seqr.stop();

  auto t_end = std::chrono::steady_clock::now();

  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t_end -
                                                                     t_start)
                   .count()
            << " ms" << std::endl;

  return 0;
}
