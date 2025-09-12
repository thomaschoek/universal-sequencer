#include "sequence/clock.hpp"

namespace MicroComposer {
namespace sequence {

inline void Sequence_clock::tick() const { cond.notify_all(); }

} // namespace sequence
} // namespace MicroComposer
