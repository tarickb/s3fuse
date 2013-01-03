#include <gtest/gtest.h>

#include "base/timer.h"

using s3::base::timer;

TEST(timer, current_time_et)
{
  double start, stop, diff;

  start = timer::get_current_time();
  sleep(1);
  stop = timer::get_current_time();

  diff = stop - start;

  EXPECT_GT(diff, 0.9);
  EXPECT_LT(diff, 1.1);
}
