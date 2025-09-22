#include "sequencer/step_sequencer.h"
#include "synth/step_synth.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace MicroComposer::sequencer;
  using namespace MicroComposer::synth;
  AtomicStepSequence seq{8};

  // Add some steps with different frequencies (musical notes)
  // Each step: offset, duration, {frequency, amplitude, phase}
  seq.push_back(Step(0.0, 0.5, {261.63, 0.6})); // C4
  seq.push_back(Step(0.0, 0.5, {293.66, 0.6})); // D4
  seq.push_back(Step(0.0, 0.5, {329.63, 0.6})); // E4
  seq.push_back(Step(0.0, 0.5, {349.23, 0.6})); // F4
  seq.push_back(Step(0.0, 0.5, {392.00, 0.6})); // G4
  seq.push_back(Step(0.0, 0.5, {440.00, 0.6})); // A4
  seq.push_back(Step(0.0, 0.5, {493.88, 0.6})); // B4
  seq.push_back(Step(0.0, 0.5, {523.25, 0.6})); // C5

  RealTimeAudioOutput synth_out;
  StepSynth synth{synth_out};
  StepSequencer seqr{seq, synth};

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
