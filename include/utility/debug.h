#ifndef NDEBUG
#ifndef MICRO_COMPOSER_DEBUG_H
#define MICRO_COMPOSER_DEBUG_H

#include <iostream>
#include <string>
#include <syncstream>

namespace Micro_composer {

namespace debug {

inline void msg(std::string msg, std::ostream& stream = std::cerr) {
  std::osyncstream(stream) << "[DEBUG] " << msg << std::endl << std::flush;
}

} // namespace debug

} // namespace Micro_composer

#endif
#endif
