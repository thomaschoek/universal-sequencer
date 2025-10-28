#ifndef MICRO_COMPOSER_SIMPLE_AUDIO_OUTPUT_H
#define MICRO_COMPOSER_SIMPLE_AUDIO_OUTPUT_H

#include <cstdint>
#include <vector>
#include <mutex>
#include <iostream>

namespace Micro_composer {

namespace audio {

// Simple audio output that writes PCM data to stdout
// Can be piped to aplay: ./MicroComposer | aplay -f S16_LE -r 44100
class Simple_audio_output {
public:
  Simple_audio_output(uint32_t sample_rate = 44100) : sample_rate_{sample_rate} {}

  // Play a vector of samples (normalized -1.0 to 1.0)
  void play_samples(const std::vector<double>& samples) {
    std::scoped_lock lck{mutex_};

    // Convert double samples to 16-bit PCM
    std::vector<int16_t> pcm_data;
    pcm_data.reserve(samples.size());

    for (double sample : samples) {
      // Clamp to [-1.0, 1.0] and convert to 16-bit
      if (sample > 1.0) sample = 1.0;
      if (sample < -1.0) sample = -1.0;
      pcm_data.push_back(static_cast<int16_t>(sample * 32767.0));
    }

    // Write PCM data to stdout
    std::cout.write(reinterpret_cast<const char*>(pcm_data.data()),
                    pcm_data.size() * sizeof(int16_t));
    std::cout.flush();
  }

private:
  uint32_t sample_rate_;
  std::mutex mutex_;
};

} // namespace audio

} // namespace Micro_composer

#endif // MICRO_COMPOSER_SIMPLE_AUDIO_OUTPUT_H
