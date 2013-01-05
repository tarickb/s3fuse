#include <gtest/gtest.h>

#include "threads/async_handle.h"

using boost::bind;
using boost::scoped_ptr;
using boost::thread;

using s3::threads::callback_async_handle;
using s3::threads::wait_async_handle;

namespace
{
  void delay_signal_handle(const wait_async_handle::ptr &h, int i)
  {
    // of course this doesn't actually guarantee that we'll signal
    // after wait() is called, but .. it's close enough

    sleep(1);

    h->complete(i);
  }

  void set_value(int i, int *r)
  {
    *r = i;
  }
}

TEST(wait_async_handle, signal_before_wait)
{
  wait_async_handle::ptr h(new wait_async_handle());

  h->complete(123);

  EXPECT_EQ(123, h->wait());
}

TEST(wait_async_handle, signal_after_wait)
{
  wait_async_handle::ptr h(new wait_async_handle());
  scoped_ptr<thread> t(new thread(bind(delay_signal_handle, h, 321)));

  EXPECT_EQ(321, h->wait());

  t->join();
}

TEST(callback_async_handle, callback)
{
  int r = 0;
  callback_async_handle::ptr h(new callback_async_handle(bind(set_value, 444, &r)));

  ASSERT_EQ(0, r); // shouldn't be set before we call complete()

  h->complete(444);

  EXPECT_EQ(444, r);
}
