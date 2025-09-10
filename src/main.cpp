#include "sequence/clock.h"
#include "sequence/output.h"

#include <chrono>
#include <iostream>

int main() {
  using namespace MicroComposer::sequence;
  auto clock_ = Sequence_clock();

  auto t_start = std::chrono::steady_clock::now();
  clock_.start();

  std::this_thread::sleep_for(std::chrono::seconds(5));
  clock_.stop();

  auto t_end = std::chrono::steady_clock::now();

  std::cout << "Elapsed time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t_end -
                                                                     t_start)
                   .count()
            << " ms" << std::endl;

  auto output_ = SequenceOutput();
  return 0;
}
