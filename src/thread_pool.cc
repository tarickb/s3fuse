/*
 * thread_pool.cc
 * -------------------------------------------------------------------------
 * Implements a pool of worker threads which are monitored for timeouts.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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
#include <boost/lexical_cast.hpp>

#include "logger.h"
#include "thread_pool.h"
#include "work_item_queue.h"
#include "worker_thread.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  BOOST_STATIC_ASSERT(PR_FG == 0);
  BOOST_STATIC_ASSERT(PR_BG == 1);

  const int POOL_COUNT = 2; // PR_FG and PR_BG
  const int NUM_THREADS_PER_POOL = 8;

  void sleep_one_second()
  {
    struct timespec ts;

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    nanosleep(&ts, NULL);
  }

  class _thread_pool
  {
  public:
    typedef shared_ptr<_thread_pool> ptr;

    _thread_pool(const string &id)
      : _queue(new work_item_queue()),
        _id(id),
        _respawn_counter(0),
        _done(false)
    {
      _watchdog_thread.reset(new thread(bind(&_thread_pool::watchdog, this)));

      for (int i = 0; i < NUM_THREADS_PER_POOL; i++)
        _threads.push_back(worker_thread::create(_queue));
    }

    ~_thread_pool()
    {
      _queue->abort();
      _done = true;

      // shut watchdog down first so it doesn't use _threads
      _watchdog_thread->join();
      _threads.clear();

      // give the threads precisely one second to clean up (and print debug info), otherwise skip them and move on
      sleep_one_second();

      S3_LOG(LOG_DEBUG, "_thread_pool::~_thread_pool", "[%s] respawn counter: %i.\n", _id.c_str(), _respawn_counter);
    }

    inline void post(const work_item::worker_function &fn, const async_handle::ptr &ah)
    {
      _queue->post(work_item(fn, ah));
    }

  private:
    typedef std::list<worker_thread::ptr> wt_list;

    void watchdog()
    {
      while (!_done) {
        int respawn = 0;

        for (wt_list::iterator itor = _threads.begin(); itor != _threads.end(); /* do nothing */) {
          if (!(*itor)->check_timeout())
            ++itor;
          else {
            respawn++;
            itor = _threads.erase(itor);
          }
        }

        for (int i = 0; i < respawn; i++)
          _threads.push_back(worker_thread::create(_queue));

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

  _thread_pool *s_pools[POOL_COUNT];
}

void thread_pool::init()
{
  for (int i = 0; i < POOL_COUNT; i++)
    s_pools[i] = new _thread_pool(string("priority ") + lexical_cast<string>(i));
}

void thread_pool::terminate()
{
  for (int i = 0; i < POOL_COUNT; i++)
    delete s_pools[i];
}

void thread_pool::internal_post(
  work_item_priority p,
  const work_item::worker_function &fn,
  const async_handle::ptr &ah)
{
  assert(p < POOL_COUNT);

  s_pools[p]->post(fn, ah);
}
