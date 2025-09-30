#include "sequencable/oscillation_event.h"
#include "sequencer/atomic_sequencer.h"
#include "sequencer/parallel_sequencer.h"
#include "synth/synth.h"

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::synth;
  using namespace Micro_composer::container;
  using namespace Micro_composer::sequencable;
  std::vector<Oscillation_event> steps;

  // Add some steps with different frequencies (musical notes)
  // Each step: offset, duration, {frequency, amplitude, phase}
  steps.push_back(Oscillation_event{261.63}); // C4
  steps.push_back(Oscillation_event{293.66}); // D4
  steps.push_back(Oscillation_event{329.63}); // E4
  steps.push_back(Oscillation_event{349.23}); // F4
  steps.push_back(Oscillation_event{392.00}); // G4
  steps.push_back(Oscillation_event{440.00}); // A4
  steps.push_back(Oscillation_event{493.88}); // B4
  steps.push_back(Oscillation_event{523.25}); // C5

  std::vector<Oscillation_event> reverse_steps;
  for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
    reverse_steps.push_back(*it);
  }

  RealTimeAudioOutput synth_out_1, synth_out_2;
  Synthesizer synth_1{synth_out_1}, synth_2{synth_out_2};
  std::function<void(Oscillation_event&&)> handler_1 =
      [&synth_1](const Oscillation_event&& event) {
        std::ignore = std::async(std::launch::async,
                                 [&synth_1, event]() { synth_1.play(event); });
      };
  decltype(handler_1) handler_2 = [&synth_2](const Oscillation_event&& event) {
    std::ignore = std::async(std::launch::async,
                             [&synth_2, event]() { synth_2.play(event); });
  };

  std::vector<std::vector<Oscillation_event>> sequences;
  sequences.emplace_back(steps);
  sequences.emplace_back(reverse_steps);

  std::vector<decltype(handler_1)> handlers;
  handlers.emplace_back(handler_1);
  handlers.emplace_back(handler_2);

  Atomic_sequencer<Oscillation_event> seqr;

  auto t_start = std::chrono::steady_clock::now();

  seqr.assign({
      Oscillation_event{261.63}, // C4
      Oscillation_event{293.66}, // D4
      Oscillation_event{329.63}, // E4
      Oscillation_event{349.23}, // F4
      Oscillation_event{392.00}, // G4
      Oscillation_event{440.00}, // A4
      Oscillation_event{493.88}, // B4
      Oscillation_event{523.25}, // C5
  });

  seqr.set_handler(handler_1);
  seqr.start();

  std::this_thread::sleep_for(std::chrono::seconds(4));
  seqr.assign({
      Oscillation_event{320.63}, // C4
      Oscillation_event{293.66}, // D4
      Oscillation_event{329.63}, // E4
      Oscillation_event{349.23}, // F4
      Oscillation_event{800.00}, // G4
      Oscillation_event{440.00}, // A4
      Oscillation_event{493.88}, // B4
      Oscillation_event{523.25}, // C5
  });
  std::this_thread::sleep_for(std::chrono::seconds(1));
  seqr.update(7, 100, 1.0, 0.1);
  // seqr.set_offset(std::chrono::milliseconds(500));
  std::this_thread::sleep_for(std::chrono::seconds(3));

  seqr.set_duration(std::chrono::milliseconds(100));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  seqr.set_duration(std::chrono::milliseconds(50));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  seqr.set_duration(std::chrono::milliseconds(10));
  std::this_thread::sleep_for(std::chrono::seconds(5));
  seqr.set_duration(std::chrono::milliseconds(300));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  seqr.set_duration(std::chrono::milliseconds(600));
  std::this_thread::sleep_for(std::chrono::seconds(5));
  seqr.set_offset(std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(5));

  seqr.stop();

  auto t_end = std::chrono::steady_clock::now();

  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t_end -
                                                                     t_start)
                   .count()
            << " ms" << std::endl;

  // Convert vector<vector<Event>> to vector<Atomic_sequencer<Event>>
  std::vector<Atomic_sequencer<Oscillation_event>> sequencers;

  Parallel_sequencer<Oscillation_event> psqr{sequences};

  psqr.set_handlers(handlers);

  psqr.start_all();

  std::this_thread::sleep_for(std::chrono::seconds(10));

  psqr.stop_all();

  return 0;
}
