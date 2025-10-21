#include "sequencable/oscillation_event.h"
#include "sequencer/sequencer_template.h"
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
  std::vector<double> frequencies1 = {261.63, 293.66, 329.63, 349.23};
  //, 392.00, 440.00, 493.88, 523.25};
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
    seq2.push_back(evt);
  }

  std::vector<std::vector<Oscillation_event>> sequences = {seq1, seq2};

  // Set up audio output - create a pool of synthesizers
  constexpr std::size_t SYNTH_POOL_SIZE = 1;
  RealTimeAudioOutput synth_output;
  Synthesizer synth{synth_output};

  auto sequencer = Sequencer<Oscillation_event>(sequences[0]);

  std::cout << "[MAIN] Starting sequencer with repeat=true\n";
  auto start_time = Sequencer<Oscillation_event>::Clock::now() +
                    std::chrono::milliseconds(50);
  sequencer.start(start_time, true);

  // Create a consumer thread that plays events from the sequencer
  std::jthread player([&sequencer, &synth](std::stop_token st) {
    while (!st.stop_requested()) {
      try {
        // Get the next event from the sequencer (blocks until ready)

        //        std::cout << "[PLAYER] Playing frequency: " << event.frequency
        //                  << " Hz, duration: "
        //                  <<
        //                  std::chrono::duration_cast<std::chrono::milliseconds>(
        //                         event.duration)
        //                         .count()
        //                  << " ms\n";
        const Oscillation_event buffer = std::async([&sequencer]() {
                                           return sequencer.await_event();
                                         }).get();
        std::ignore = std::async([&synth, buffer]() { synth.play(buffer); });
      } catch (const std::runtime_error& e) {
        std::cout << "[PLAYER] " << e.what() << "\n";
        break;
      }
    }
    std::cout << "[PLAYER] Consumer thread stopped\n";
  });

  std::cout << "[MAIN] Playing for 10 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "[MAIN] Pausing sequencer\n";
  sequencer.pause();

  std::cout << "[MAIN] Waiting 2 seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "[MAIN] Restarting sequencer\n";
  auto restart_time = Sequencer<Oscillation_event>::Clock::now() +
                      std::chrono::milliseconds(50);
  sequencer.start(restart_time, true);

  std::cout << "[MAIN] Playing for 5 more seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "[MAIN] Stopping sequencer\n";
  sequencer.stop();

  std::cout << "[MAIN] Stopping player thread\n";
  player.request_stop();

  if (player.joinable()) {
    player.join();
  }

  std::cout << "[MAIN] Done!\n";
}
