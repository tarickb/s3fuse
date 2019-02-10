#include <gtest/gtest.h>

#include "base/timer.h"

namespace s3 {
namespace base {
namespace tests {

TEST(Timer, Sleep) {
  double start = Timer::GetCurrentTime();
  Timer::Sleep(1);
  double stop = Timer::GetCurrentTime();

  double diff = stop - start;

  EXPECT_GT(diff, 0.9);
  EXPECT_LT(diff, 1.1);
}

TEST(Timer, CurrentTimeEt) {
  double start = Timer::GetCurrentTime();
  sleep(1);
  double stop = Timer::GetCurrentTime();

  double diff = stop - start;

  EXPECT_GT(diff, 0.9);
  EXPECT_LT(diff, 1.1);
}

}  // namespace tests
}  // namespace base
}  // namespace s3
