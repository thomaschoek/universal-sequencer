#include "synth/synth_output.h"
#include <cstdio>
#ifndef NDEBUG
#include <iostream>
#endif

namespace Micro_composer {

namespace synth {

void SynthOutput::write(const std::vector<double>& samples) {
  // Default implementation does nothing
  (void)samples; // Suppress unused parameter warning
}

RealTimeAudioOutput::RealTimeAudioOutput(double sample_rate)
    : pacat_pipe(nullptr), is_open(false) {
  // Open pipe to pacat for real-time audio output
  char command[256];
  snprintf(command, sizeof(command),
           "pacat --rate=%.0f --format=s16le --channels=1", sample_rate);

  pacat_pipe = popen(command, "w");
  if (pacat_pipe) {
    is_open = true;
  }

#ifndef NDEBUG
  if (pacat_pipe) {
    std::cout << "[AUDIO] Real-time audio output initialized" << std::endl;
  } else {
    std::cout << "[AUDIO] Failed to initialize audio output" << std::endl;
  }
#endif
}

RealTimeAudioOutput::~RealTimeAudioOutput() {
  if (pacat_pipe) {
    pclose(pacat_pipe);
  }
}

void RealTimeAudioOutput::write(const std::vector<double>& samples) {
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

} // namespace synth

} // namespace Micro_composer
