/*
 * thread_pool.h
 * -------------------------------------------------------------------------
 * Interface for asynchronous operations (post, wait) on a thread pool.
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

#ifndef S3_THREAD_POOL_H
#define S3_THREAD_POOL_H

#include "async_handle.h"
#include "work_item.h"

namespace s3
{
  enum work_item_priority
  {
    PR_FG,
    PR_BG,
    PR_NO_REQ // TODO: implement this
  };

  class thread_pool
  {
  public:
    typedef work_item::worker_function worker_function;

    static void init();
    static void terminate();

    inline static wait_async_handle::ptr post(
      work_item_priority p,
      const worker_function &fn)
    {
      wait_async_handle::ptr ah(new wait_async_handle());

      internal_post(p, fn, ah);

      return ah;
    }

    inline static void post(
      work_item_priority p,
      const worker_function &fn, 
      const callback_async_handle::callback_function &cb)
    {
      internal_post(
        p,
        fn,
        async_handle::ptr(new callback_async_handle(cb)));
    }

    inline static int call(work_item_priority p, const worker_function &fn)
    {
      return post(p, fn)->wait();
    }

    inline static void call_async(work_item_priority p, const worker_function &fn)
    {
      post(p, fn);
    }

  private:
    static void internal_post(
      work_item_priority p, 
      const worker_function &fn, 
      const async_handle::ptr &ah);
  };
}

#endif
