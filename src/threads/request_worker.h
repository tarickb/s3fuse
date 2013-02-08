/*
 * threads/request_worker.h
 * -------------------------------------------------------------------------
 * Pool worker thread with attached request object.
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

#ifndef S3_THREADS_REQUEST_WORKER_H
#define S3_THREADS_REQUEST_WORKER_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "threads/work_item.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace threads
  {
    class async_handle;
    class work_item_queue;

    class request_worker
    {
    public:
      typedef boost::shared_ptr<request_worker> ptr;

      static ptr create(const boost::shared_ptr<work_item_queue> &queue)
      {
        ptr wt(new request_worker(queue));

        // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
        wt->_thread.reset(new boost::thread(boost::bind(&request_worker::work, wt)));

        return wt;
      }

      ~request_worker();

      bool check_timeout(); // return true if thread has hanged

    private:
      request_worker(const boost::shared_ptr<work_item_queue> &queue);

      void work();

      boost::mutex _mutex;
      boost::shared_ptr<boost::thread> _thread;
      boost::shared_ptr<base::request> _request;
      double _time_in_function, _time_in_request;

      // access controlled by _mutex
      boost::weak_ptr<work_item_queue> _queue;
      work_item _current_item;
    };
  }
}

#endif
