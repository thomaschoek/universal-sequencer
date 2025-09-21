#include "sequencer/step_sequencer.h"
#include "sequencer/step_sequencer_output.h"
#include "synth/synth.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

using namespace MicroComposer::sequencer;
using namespace MicroComposer::synth;

// Simple WAV file writer for demonstration
void writeWavFile(const std::vector<double> &samples, double sample_rate,
                  const std::string &filename) {
  std::ofstream file(filename, std::ios::binary);

  // WAV header (44 bytes)
  const int num_samples = samples.size();
  const int byte_rate = sample_rate * 2; // 16-bit mono
  const int data_size = num_samples * 2;
  const int file_size = 36 + data_size;

  // RIFF header
  file.write("RIFF", 4);
  file.write(reinterpret_cast<const char *>(&file_size), 4);
  file.write("WAVE", 4);

  // Format chunk
  file.write("fmt ", 4);
  int chunk_size = 16;
  file.write(reinterpret_cast<const char *>(&chunk_size), 4);
  short audio_format = 1; // PCM
  file.write(reinterpret_cast<const char *>(&audio_format), 2);
  short num_channels = 1; // mono
  file.write(reinterpret_cast<const char *>(&num_channels), 2);
  int sr = static_cast<int>(sample_rate);
  file.write(reinterpret_cast<const char *>(&sr), 4);
  file.write(reinterpret_cast<const char *>(&byte_rate), 4);
  short block_align = 2;
  file.write(reinterpret_cast<const char *>(&block_align), 2);
  short bits_per_sample = 16;
  file.write(reinterpret_cast<const char *>(&bits_per_sample), 2);

  // Data chunk
  file.write("data", 4);
  file.write(reinterpret_cast<const char *>(&data_size), 4);

  // Audio data (convert to 16-bit)
  for (double sample : samples) {
    short sample_16 = static_cast<short>(sample * 32767.0);
    file.write(reinterpret_cast<const char *>(&sample_16), 2);
  }

  file.close();
}

int main() {
  std::cout << "=== Audio Output Example ===" << std::endl;

  // Create a simple melody
  AtomicStepSequence sequence(0);

  // A simple melody with longer notes so we can hear them
  sequence.push_back(Step(0.0, 1.0, {261.63, 0.8})); // C4 - 1 second
  sequence.push_back(Step(0.0, 1.0, {293.66, 0.8})); // D4
  sequence.push_back(Step(0.0, 1.0, {329.63, 0.8})); // E4
  sequence.push_back(Step(0.0, 1.0, {349.23, 0.8})); // F4

  // Create synthesizer output
  StepSequencerSynthOutput synth_output(44100.0, WaveformType::SINE);

  // Collect all audio samples
  std::vector<double> all_samples;

  synth_output.setAudioCallback(
      [&all_samples](const std::vector<double> &samples, double sample_rate) {
        std::cout << "[AUDIO] Capturing " << samples.size() << " samples..."
                  << std::endl;
        all_samples.insert(all_samples.end(), samples.begin(), samples.end());
      });

  // Create step sequencer with synth output
  StepSequencer sequencer(sequence, synth_output);

  std::cout << "Generating audio..." << std::endl;
  sequencer.start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  sequencer.stop();

  std::cout << "Writing WAV file..." << std::endl;
  writeWavFile(all_samples, 44100.0, "/tmp/synth_output.wav");

  std::cout << "Audio saved to /tmp/synth_output.wav" << std::endl;
  std::cout << "You can play it with: aplay /tmp/synth_output.wav" << std::endl;
  std::cout << "Or: vlc /tmp/synth_output.wav" << std::endl;
  std::cout << "Or: paplay /tmp/synth_output.wav" << std::endl;

  // Try to play automatically if aplay is available
  std::cout << "\nAttempting to play automatically..." << std::endl;
  int result = system("aplay /tmp/synth_output.wav 2>/dev/null");
  if (result != 0) {
    result = system("paplay /tmp/synth_output.wav 2>/dev/null");
    if (result != 0) {
      std::cout << "Could not auto-play. Please play the file manually."
                << std::endl;
    }
  }

  return 0;
}