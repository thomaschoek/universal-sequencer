#include "sequencer/synth_sequencer.h"
#include "synth/synth.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace MicroComposer::sequencer;
using namespace MicroComposer::synth;

// Real-time audio output using PulseAudio via pacat
class RealTimeAudioOutput {
private:
  FILE *pacat_pipe;
  bool is_open;

public:
  RealTimeAudioOutput(double sample_rate = 44100.0)
      : pacat_pipe(nullptr), is_open(false) {
    // Open pipe to pacat for real-time audio output
    char command[256];
    snprintf(command, sizeof(command),
             "pacat --rate=%.0f --format=s16le --channels=1", sample_rate);

    pacat_pipe = popen(command, "w");
    if (pacat_pipe) {
      is_open = true;
      std::cout << "[AUDIO] Real-time audio output initialized" << std::endl;
    } else {
      std::cout << "[AUDIO] Failed to initialize audio output" << std::endl;
    }
  }

  ~RealTimeAudioOutput() {
    if (pacat_pipe) {
      pclose(pacat_pipe);
    }
  }

  void playAudio(const std::vector<double> &samples, double sample_rate) {
    if (!is_open || !pacat_pipe)
      return;

    // Convert double samples to 16-bit signed integers and write to pipe
    for (double sample : samples) {
      // Clamp to [-1.0, 1.0] and convert to 16-bit
      sample = std::max(-1.0, std::min(1.0, sample));
      int16_t sample_16 = static_cast<int16_t>(sample * 32767.0);

      fwrite(&sample_16, sizeof(int16_t), 1, pacat_pipe);
    }
    fflush(pacat_pipe);
  }

  bool isWorking() const { return is_open; }
};

int main() {
  std::cout << "=== Real-Time Synthesizer Example ===" << std::endl;

  // Initialize real-time audio output
  RealTimeAudioOutput audio_output(44100.0);

  if (!audio_output.isWorking()) {
    std::cout << "Error: Could not initialize audio output." << std::endl;
    std::cout << "Make sure PulseAudio is running and pacat is available."
              << std::endl;
    return 1;
  }

  // Create a sequence with musical notes
  AtomicStepSequence sequence(0);

  // Add some steps with different frequencies (musical notes)
  // Each step: offset, duration, {frequency, amplitude, phase}
  sequence.push_back(Step(0.0, 0.5, {261.63, 0.6})); // C4
  sequence.push_back(Step(0.0, 0.5, {293.66, 0.6})); // D4
  sequence.push_back(Step(0.0, 0.5, {329.63, 0.6})); // E4
  sequence.push_back(Step(0.0, 0.5, {349.23, 0.6})); // F4
  sequence.push_back(Step(0.0, 0.5, {392.00, 0.6})); // G4
  sequence.push_back(Step(0.0, 0.5, {440.00, 0.6})); // A4
  sequence.push_back(Step(0.0, 0.5, {493.88, 0.6})); // B4
  sequence.push_back(Step(0.0, 0.5, {523.25, 0.6})); // C5

  // Create a synthesizer sequencer
  SynthSequencer synth_seq(sequence, 44100.0, WaveformType::SINE);

  // Set up real-time audio callback
  synth_seq.setAudioCallback(
      [&audio_output](const std::vector<double> &samples, double sample_rate) {
        audio_output.playAudio(samples, sample_rate);
      });

  std::cout << "🎵 Playing C major scale with sine wave..." << std::endl;
  synth_seq.start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  synth_seq.stop();

  std::cout << "🎵 Switching to square wave..." << std::endl;
  synth_seq.setWaveform(WaveformType::SQUARE);
  synth_seq.start();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  synth_seq.stop();

  std::cout << "🎵 Switching to sawtooth wave..." << std::endl;
  synth_seq.setWaveform(WaveformType::SAWTOOTH);
  synth_seq.start();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  synth_seq.stop();

  std::cout << "🎵 Switching to triangle wave..." << std::endl;
  synth_seq.setWaveform(WaveformType::TRIANGLE);
  synth_seq.start();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  synth_seq.stop();

  std::cout << "\n✅ Real-Time Synthesizer Demo Complete!" << std::endl;
  return 0;
}