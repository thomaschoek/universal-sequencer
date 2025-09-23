#include "sequencable/oscillation_event.h"
#include "sequencer/atomic_sequencer.h"
#include "sequencer/parallel_sequencer.tpp"
#include "synth/synth.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::synth;
  using namespace Micro_composer::atomic_deque;
  using namespace Micro_composer::sequencable;
  Atomic_deque<OscillationEvent> steps;

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

  Atomic_deque<OscillationEvent> reverse_steps;
  for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
    reverse_steps.push_back(*it);
  }

  RealTimeAudioOutput synth_out_1, synth_out_2;
  Synthesizer synth_1{synth_out_1}, synth_2{synth_out_2};
  auto handler_1 = [&synth_1](const OscillationEvent& event) {
    synth_1.play(event);
  };
  Atomic_sequencer<OscillationEvent, decltype(handler_1)> seqr_1{handler_1,
                                                                 steps};
  Atomic_sequencer<OscillationEvent, decltype(handler_1)> seqr_2{handler_1,
                                                                 reverse_steps};

  Atomic_deque<Atomic_sequencer<OscillationEvent, decltype(handler_1)>>
      sequencers;
  sequencers.push_back(seqr_1);
  sequencers.push_back(seqr_2);

  Parallel_sequencer<OscillationEvent, decltype(handler_1)> seqr{sequencers};

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
