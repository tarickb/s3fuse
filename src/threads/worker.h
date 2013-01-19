/*
 * threads/worker.h
 * -------------------------------------------------------------------------
 * Pool worker thread.
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

#ifndef S3_THREADS_WORKER_H
#define S3_THREADS_WORKER_H

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  namespace threads
  {
    class work_item_queue;

    class worker
    {
    public:
      typedef boost::shared_ptr<worker> ptr;

      static ptr create(const boost::shared_ptr<work_item_queue> &queue)
      {
        ptr wt(new worker(queue));

        // passing "wt", a shared_ptr, for "this" keeps the object alive so long as worker() hasn't returned
        wt->_thread.reset(new boost::thread(boost::bind(&worker::work, wt)));

        return wt;
      }


      inline bool check_timeout() const { return false; }

    private:
      inline worker(const boost::shared_ptr<work_item_queue> &queue)
        : _queue(queue)
      {
      }

      void work();

      boost::shared_ptr<boost::thread> _thread;
      boost::shared_ptr<work_item_queue> _queue;
    };
  }
}

#endif
