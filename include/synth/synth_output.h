#ifndef MICRO_COMPOSER_SYNTH_OUTPUT_H
#define MICRO_COMPOSER_SYNTH_OUTPUT_H

#include <cstdio>
#include <vector>

namespace Micro_composer {

namespace synth {

class SynthOutput {
protected:
  double sample_rate_{44100.0};

  explicit SynthOutput(double sample_rate = 44100.0)
      : sample_rate_(sample_rate) {}

public:
  const double get_sample_rate() const { return sample_rate_; }
  virtual void write(const std::vector<double>& samples);
};

class RealTimeAudioOutput : public SynthOutput {
private:
  FILE* pacat_pipe;
  bool is_open;

public:
  explicit RealTimeAudioOutput(double sample_rate = 44100.0);

  ~RealTimeAudioOutput();

  void write(const std::vector<double>& samples) override;

  bool isWorking() const { return is_open; }
};

} // namespace synth

} // namespace Micro_composer

#endif // MICRO_COMPOSER_SYNTH_OUTPUT_H
