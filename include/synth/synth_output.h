#ifndef MICRO_COMPOSER_SYNTH_OUTPUT_H
#define MICRO_COMPOSER_SYNTH_OUTPUT_H

#include <cstdio>
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
  explicit RealTimeAudioOutput(double sample_rate = 44100.0);

  ~RealTimeAudioOutput();

  void write(const std::vector<double> &samples);

  bool isWorking() const { return is_open; }
};

} // namespace synth

} // namespace MicroComposer

#endif // MICRO_COMPOSER_SYNTH_OUTPUT_H
