#ifndef MICRO_COMPOSER_SYNTH_OUTPUT_H
#define MICRO_COMPOSER_SYNTH_OUTPUT_H

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <vector>

namespace MicroComposer {

namespace synth {

class SynthOutput {
public:
  const double get_sample_rate() const;
  void write(const std::vector<double> samples);
};

class RealTimeAudioOutput : public SynthOutput {
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

  void write(const std::vector<double> &samples) {
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

} // namespace synth

} // namespace MicroComposer

#endif // MICRO_COMPOSER_SYNTH_OUTPUT_H
