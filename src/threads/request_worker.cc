/*
 * threads/request_worker.cc
 * -------------------------------------------------------------------------
 * Pool worker thread with attached request (implementation).
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

#include <atomic>
#include <iomanip>

#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/timer.h"
#include "services/service.h"
#include "threads/async_handle.h"
#include "threads/request_worker.h"
#include "threads/work_item_queue.h"

namespace s3
{
  namespace threads
  {

namespace
{
  double s_total_req_time = 0.0;
  double s_total_fn_time = 0.0;

  std::mutex s_stats_mutex;
  std::atomic_int s_reposted_items(0);

  void statistics_writer(std::ostream *o)
  {
    o->setf(std::ostream::fixed);

    *o <<
      "thread pool request workers:\n"
      "  total request time: " << std::setprecision(3) << s_total_req_time << " s\n"
      "  total function time: " << s_total_fn_time << " s\n"
      "  request wait: " << std::setprecision(2) << s_total_req_time / s_total_fn_time * 100.0 << " %\n"
      "  reposted items: " << s_reposted_items << "\n";
  }

  base::statistics::writers::entry s_writer(statistics_writer, 0);
}

request_worker::request_worker(const work_item_queue::ptr &queue)
  : _request(new base::request()),
    _time_in_function(0.),
    _time_in_request(0.),
    _queue(queue)
{
  _request->set_hook(services::service::get_request_hook());
}

request_worker::~request_worker()    
{
  if (_time_in_function > 0.0) {
    std::lock_guard<std::mutex> lock(s_stats_mutex);

    s_total_req_time += _time_in_request;
    s_total_fn_time += _time_in_function;
  }
}

bool request_worker::check_timeout()
{
  std::lock_guard<std::mutex> lock(_mutex);

  if (_request->check_timeout()) {
    work_item_queue::ptr queue = _queue.lock();

    if (queue && _current_item.has_retries_left()) {
      ++s_reposted_items;
      queue->post(_current_item.decrement_retry_counter());
    } else {
      _current_item.get_ah()->complete(-ETIMEDOUT);
    }

    // prevent worker() from continuing
    _queue.reset();
    _current_item = work_item();

    return true;
  }

  return false;
}

void request_worker::work()
{
  while (true) {
    std::unique_lock<std::mutex> lock(_mutex);
    work_item_queue::ptr queue;
    work_item item;
    int r;

    // the interplay between _mutex and _queue is a little (a lot?) ugly here, but the principles are:
    //
    // 1a. we don't want to hold _mutex while also keeping _queue alive.
    // 1b. we want to minimize the amount of time we keep _queue alive.
    // 2.  we need to lock _mutex when reading/writing _queue or _current_ah (because check_timeout does too).

    queue = _queue.lock();
    lock.unlock();

    if (!queue)
      break;

    item = queue->get_next();
    queue.reset();

    if (!item.is_valid())
      break;

    lock.lock();
    _current_item = item;
    lock.unlock();

    try {
      double start_time, end_time;

      start_time = base::timer::get_current_time();
      _request->reset_current_run_time();

      r = item.get_function()(_request);

      end_time = base::timer::get_current_time();
      _time_in_function += end_time - start_time;
      _time_in_request += _request->get_current_run_time();

    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "request_worker::work", "caught exception: %s\n", e.what());
      r = -ECANCELED;

    } catch (...) {
      S3_LOG(LOG_WARNING, "request_worker::work", "caught unknown exception.\n");
      r = -ECANCELED;
    }

    lock.lock();

    if (_current_item.is_valid()) {
      _current_item.get_ah()->complete(r);

      _current_item = work_item();
    }
  }

  // the thread in _thread holds a shared_ptr to this, and will keep it from being destructed
  _thread.reset();
}

  }
}
