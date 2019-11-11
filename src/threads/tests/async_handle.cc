#include <functional>
#include <thread>

#include <gtest/gtest.h>

#include "threads/async_handle.h"

namespace s3 {
namespace threads {
namespace tests {

namespace {
void DelaySignalHandle(AsyncHandle *h, int i) {
  // of course this doesn't actually guarantee that we'll signal
  // after Wait() is called, but .. it's close enough
  sleep(1);
  h->Complete(i);
}
}  // namespace

TEST(AsyncHandle, SignalBeforeWait) {
  AsyncHandle h;
  h.Complete(123);
  EXPECT_EQ(123, h.Wait());
}

TEST(AsyncHandle, SignalAfterWait) {
  AsyncHandle h;
  std::thread t(std::bind(DelaySignalHandle, &h, 321));
  EXPECT_EQ(321, h.Wait());
  t.join();
}

}  // namespace tests
}  // namespace threads
}  // namespace s3
