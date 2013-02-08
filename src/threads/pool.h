/*
 * threads/pool.h
 * -------------------------------------------------------------------------
 * Interface for asynchronous operations (post, wait) on a thread pool.
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

#ifndef S3_THREADS_POOL_H
#define S3_THREADS_POOL_H

#include "threads/async_handle.h"
#include "threads/work_item.h"

namespace s3
{
  namespace threads
  {
    enum pool_id
    {
      PR_0 = 0,
      PR_REQ_0 = 1,
      PR_REQ_1 = 2
    };

    class pool
    {
    public:
      typedef work_item::worker_function worker_function;

      static const int DEFAULT_TIMEOUT_RETRIES = -1;

      static void init();
      static void terminate();

      inline static wait_async_handle::ptr post(
        pool_id p,
        const worker_function &fn,
        int timeout_retries = DEFAULT_TIMEOUT_RETRIES)
      {
        wait_async_handle::ptr ah(new wait_async_handle());

        internal_post(p, fn, ah, timeout_retries);

        return ah;
      }

      inline static void post(
        pool_id p,
        const worker_function &fn, 
        const callback_async_handle::callback_function &cb,
        int timeout_retries = DEFAULT_TIMEOUT_RETRIES)
      {
        internal_post(
          p,
          fn,
          async_handle::ptr(new callback_async_handle(cb)),
          timeout_retries);
      }

      inline static int call(
        pool_id p, 
        const worker_function &fn, 
        int timeout_retries = DEFAULT_TIMEOUT_RETRIES)
      {
        return post(p, fn, timeout_retries)->wait();
      }

      inline static void call_async(
        pool_id p, 
        const worker_function &fn, 
        int timeout_retries = DEFAULT_TIMEOUT_RETRIES)
      {
        post(p, fn, timeout_retries);
      }

    private:
      static void internal_post(
        pool_id p, 
        const worker_function &fn, 
        const async_handle::ptr &ah,
        int timeout_retries);
    };
  }
}

#endif
