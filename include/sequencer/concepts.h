#ifndef MICRO_COMPOSER_SEQUENCER_CONCEPTS_H
#define MICRO_COMPOSER_SEQUENCER_CONCEPTS_H

#include <concepts>

namespace Micro_composer {

namespace sequencer {

// Concept to ensure a type is a valid sequencer
template <typename T>
concept Sequencer = requires(T sequencer) {
  { sequencer.start() } -> std::same_as<void>;
  { sequencer.stop() } -> std::same_as<void>;
  { sequencer.is_running() } -> std::same_as<bool>;
  // Add more required methods as needed
};

} // namespace sequencer

} // namespace Micro_composer

#endif
