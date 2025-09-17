#include "sequencer.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace sequencer;
  Sequencer seq;

  auto t_start = std::chrono::steady_clock::now();
  seq.start();

  std::this_thread::sleep_for(std::chrono::seconds(5));
  seq.stop();

  auto t_end = std::chrono::steady_clock::now();

  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t_end -
                                                                     t_start)
                   .count()
            << " ms" << std::endl;

  return 0;
}
