#include "sequencable/premade_samples.h"
#include "sequencer/sequencer_template.h"
#include "synth/synth.h"
#include <chrono>
#include <future>
#include <iostream>
#include <map>

int main(int argc, char** argv) {

  using namespace Micro_composer;
  using namespace Micro_composer::sequencer;
  using namespace Micro_composer::sequencable;
  using namespace Micro_composer::synth;

  using Sequencer = Sequencer<Premade_samples>;
  using Sequence = Sequencer::Container;

  // Vector of note frequencies from 1=A0 to 88=C8
  static const std::vector<double> freqs{
      27.50,   29.14,   30.87,   32.70,   34.65,   36.71,   38.89,   41.20,
      43.65,   46.25,   49.00,   51.91,   55.00,   58.27,   61.74,   65.41,
      69.30,   73.42,   77.78,   82.41,   87.31,   92.50,   98.00,   103.83,
      110.00,  116.54,  123.47,  130.81,  138.59,  146.83,  155.56,  164.81,
      174.61,  185.00,  196.00,  207.65,  220.00,  233.08,  246.94,  261.63,
      277.18,  293.66,  311.13,  329.63,  349.23,  369.99,  392.00,  415.30,
      440.00,  466.16,  493.88,  523.25,  554.37,  587.33,  622.25,  659.25,
      698.46,  739.99,  783.99,  830.61,  880.00,  932.33,  987.77,  1046.50,
      1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22,
      1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02, 2637.02,
      2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01,
      4434.92, 4698.63, 4978.03};
  assert(freqs.size() == 91);
  // Map of musical note names to frequencies (in Hz)
  static const std::map<std::string, double> notes{
      {"A0", freqs[0]},   {"A#0", freqs[1]},  {"B0", freqs[2]},
      {"C1", freqs[3]},   {"C#1", freqs[4]},  {"D1", freqs[5]},
      {"D#1", freqs[6]},  {"E1", freqs[7]},   {"F1", freqs[8]},
      {"F#1", freqs[9]},  {"G1", freqs[10]},  {"G#1", freqs[11]},
      {"A1", freqs[12]},  {"A#1", freqs[13]}, {"B1", freqs[14]},
      {"C2", freqs[15]},  {"C#2", freqs[16]}, {"D2", freqs[17]},
      {"D#2", freqs[18]}, {"E2", freqs[19]},  {"F2", freqs[20]},
      {"F#2", freqs[21]}, {"G2", freqs[22]},  {"G#2", freqs[23]},
      {"A2", freqs[24]},  {"A#2", freqs[25]}, {"B2", freqs[26]},
      {"C3", freqs[27]},  {"C#3", freqs[28]}, {"D3", freqs[29]},
      {"D#3", freqs[30]}, {"E3", freqs[31]},  {"F3", freqs[32]},
      {"F#3", freqs[33]}, {"G3", freqs[34]},  {"G#3", freqs[35]},
      {"A3", freqs[36]},  {"A#3", freqs[37]}, {"B3", freqs[38]},
      {"C4", freqs[39]},  {"C#4", freqs[40]}, {"D4", freqs[41]},
      {"D#4", freqs[42]}, {"E4", freqs[43]},  {"F4", freqs[44]},
      {"F#4", freqs[45]}, {"G4", freqs[46]},  {"G#4", freqs[47]},
      {"A4", freqs[48]},  {"A#4", freqs[49]}, {"B4", freqs[50]},
      {"C5", freqs[51]},  {"C#5", freqs[52]}, {"D5", freqs[53]},
      {"D#5", freqs[54]}, {"E5", freqs[55]},  {"F5", freqs[56]},
      {"F#5", freqs[57]}, {"G5", freqs[58]},  {"G#5", freqs[59]},
      {"A5", freqs[60]},  {"A#5", freqs[61]}, {"B5", freqs[62]},
      {"C6", freqs[63]},  {"C#6", freqs[64]}, {"D6", freqs[65]},
      {"D#6", freqs[66]}, {"E6", freqs[67]},  {"F6", freqs[68]},
      {"F#6", freqs[69]}, {"G6", freqs[70]},  {"G#6", freqs[71]},
      {"A6", freqs[72]},  {"A#6", freqs[73]}, {"B6", freqs[74]},
      {"C7", freqs[75]},  {"C#7", freqs[76]}, {"D7", freqs[77]},
      {"D#7", freqs[78]}, {"E7", freqs[79]},  {"F7", freqs[80]},
      {"F#7", freqs[81]}, {"G7", freqs[82]},  {"G#7", freqs[83]},
      {"A7", freqs[84]},  {"A#7", freqs[85]}, {"B7", freqs[86]},
      {"C8", freqs[87]}};
  assert(notes.size() == 88);
  // Create example sequences with vector events
  // Each event has 3 parameters: [frequency, amplitude, phase]
  std::vector<Sequence> sequences;
  {
    Sequence seq1;
    //, 392.00, 440.00, 493.88, 523.25};
    for (auto freq : {freqs[39], freqs[51], freqs[39], freqs[43]}) {
      auto evt = std::make_unique<Premade_samples>(
          freq, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq1.push_back(std::move(evt));
    }

    Sequence seq2;
    for (auto freq : {freqs[39], freqs[43], freqs[39], freqs[51]}) {
      auto evt = std::make_unique<Premade_samples>(
          freq, 0.5, 0.0, std::chrono::milliseconds{0},
          std::chrono::milliseconds{100});
      seq2.push_back(std::move(evt));
    }

    //     seq1[2]->duration = std::chrono::milliseconds{250};
    //     seq1[2]->samples = Premade_samples::generate_sine_wave(
    //         seq1[2]->frequency, seq1[2]->amplitude, seq1[2]->phase,
    //         seq1[2]->duration, Premade_samples::default_sample_rate);
    //     seq2[2]->duration = std::chrono::milliseconds{250};
    //     seq2[2]->samples = Premade_samples::generate_sine_wave(
    //         seq2[2]->frequency, seq2[2]->amplitude, seq2[2]->phase,
    //         seq2[2]->duration, Premade_samples::default_sample_rate);
    //     seq2[6]->duration = std::chrono::milliseconds{250};
    //     seq2[6]->samples = Premade_samples::generate_sine_wave(
    //         seq2[6]->frequency, seq2[6]->amplitude, seq2[6]->phase,
    //         seq2[6]->duration, Premade_samples::default_sample_rate);
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
        Sequencer::Handler{[&synths, idx](const Premade_samples& event) {
          synths[idx]->write(event.samples_);
        }});
  }

  Sequencer::Handler proxy_handler{[&handlers](Premade_samples&& evt) {
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
      evt->samples_ = Premade_samples::generate_sine_wave(*evt);
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
