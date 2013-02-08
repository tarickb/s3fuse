/*
 * threads/pool.cc
 * -------------------------------------------------------------------------
 * Implements a pool of worker threads which are monitored for timeouts.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <list>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "threads/request_worker.h"
#include "threads/pool.h"
#include "threads/work_item_queue.h"
#include "threads/worker.h"

using boost::bind;
using boost::scoped_ptr;
using boost::thread;
using std::string;

using s3::base::config;
using s3::base::statistics;
using s3::threads::async_handle;
using s3::threads::pool;
using s3::threads::work_item;
using s3::threads::work_item_queue;

namespace
{
  BOOST_STATIC_ASSERT(s3::threads::PR_0 == 0);
  BOOST_STATIC_ASSERT(s3::threads::PR_REQ_0 == 1);
  BOOST_STATIC_ASSERT(s3::threads::PR_REQ_1 == 2);

  const int POOL_COUNT = 3; // PR_0, PR_REQ_0, PR_REQ_1
  const int NUM_THREADS_PER_POOL = 8;

  void sleep_one_second()
  {
    struct timespec ts;

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    nanosleep(&ts, NULL);
  }

  class _pool
  {
  public:
    virtual ~_pool()
    {
    }

    virtual void post(const work_item::worker_function &fn, const async_handle::ptr &ah, int timeout_retries) = 0;
  };

  template <class worker_type, bool use_watchdog>
  class _pool_impl : public _pool
  {
  public:
    _pool_impl(const string &id)
      : _queue(new work_item_queue()),
        _id(id),
        _respawn_counter(0),
        _done(false)
    {
      if (use_watchdog)
        _watchdog_thread.reset(new thread(bind(&_pool_impl::watchdog, this)));

      for (int i = 0; i < NUM_THREADS_PER_POOL; i++)
        _threads.push_back(worker_type::create(_queue));
    }

    ~_pool_impl()
    {
      _queue->abort();
      _done = true;

      if (use_watchdog) {
        // shut watchdog down first so it doesn't use _threads
        _watchdog_thread->join();
      }

      _threads.clear();

      // give the threads precisely one second to clean up (and print debug info), otherwise skip them and move on
      sleep_one_second();

      if (use_watchdog)
        statistics::write(
          "pool",
          _id,
          "\n  respawn_counter: %i",
          _respawn_counter);
    }

    virtual void post(const work_item::worker_function &fn, const async_handle::ptr &ah, int timeout_retries)
    {
      _queue->post(work_item(fn, ah, timeout_retries));
    }

  private:
    typedef std::list<typename worker_type::ptr> wt_list;

    void watchdog()
    {
      while (!_done) {
        int respawn = 0;

        for (typename wt_list::iterator itor = _threads.begin(); itor != _threads.end(); /* do nothing */) {
          if (!(*itor)->check_timeout())
            ++itor;
          else {
            respawn++;
            itor = _threads.erase(itor);
          }
        }

        for (int i = 0; i < respawn; i++)
          _threads.push_back(worker_type::create(_queue));

        _respawn_counter += respawn;
        sleep_one_second();
      }
    }

    work_item_queue::ptr _queue;
    wt_list _threads;
    scoped_ptr<thread> _watchdog_thread;
    string _id;
    int _respawn_counter;
    bool _done;
  };

  _pool *s_pools[POOL_COUNT];
}

void pool::init()
{
  s_pools[PR_0] = new _pool_impl<worker, false>("PR_0");
  s_pools[PR_REQ_0] = new _pool_impl<request_worker, true>("PR_REQ_0");
  s_pools[PR_REQ_1] = new _pool_impl<request_worker, true>("PR_REQ_1");
}

void pool::terminate()
{
  for (int i = 0; i < POOL_COUNT; i++)
    delete s_pools[i];
}

void pool::internal_post(
  pool_id p,
  const work_item::worker_function &fn,
  const async_handle::ptr &ah,
  int timeout_retries)
{
  assert(p < POOL_COUNT);

  s_pools[p]->post(
    fn, 
    ah,
    (timeout_retries == DEFAULT_TIMEOUT_RETRIES) ? config::get_timeout_retries() : timeout_retries);
}
