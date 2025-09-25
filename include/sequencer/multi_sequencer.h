#ifndef MICRO_COMPOSER_MULTI_SEQUENCER_H
#define MICRO_COMPOSER_MULTI_SEQUENCER_H

#include "sequencer/sequencer_base.h"
#include <memory>

namespace Micro_composer {

namespace sequencer {

class Multi_sequencer {
public:
  using Sequencer_clock = Sequencer_base::clock;
  using Sequencer_time_point = Sequencer_base::time_point;
  using Sequencer_vec = std::vector<std::unique_ptr<Sequencer_base>>;

  void start(Sequencer_time_point = Sequencer_clock::now());
  void stop();

  void add_sequence(std::unique_ptr<Sequencer_base>&& seq);

private:
  // Sequencers managed by this multi-sequencer that run in parallel
  Sequencer_vec sequencers_;
};

} // namespace sequencer
} // namespace Micro_composer

#endif
