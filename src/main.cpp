#include "sequencable/oscillation_event.h"
#include "sequencer/atomic_sequencer.h"
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

  RealTimeAudioOutput synth_out;
  Synthesizer synth{synth_out};
  auto handler = [&synth](const OscillationEvent& event) { synth.play(event); };
  Atomic_sequencer<OscillationEvent, decltype(handler)> seqr{handler, steps};

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
