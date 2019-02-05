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

#include <functional>
#include <memory>
#include <thread>

namespace s3 {
namespace threads {
class work_item_queue;

class worker {
public:
  typedef std::shared_ptr<worker> ptr;

  static ptr create(const std::shared_ptr<work_item_queue> &queue) {
    ptr wt(new worker(queue));

    // passing "wt", a shared_ptr, for "this" keeps the object alive so long as
    // worker() hasn't returned
    wt->_thread.reset(new std::thread(std::bind(&worker::work, wt)));

    return wt;
  }

  inline bool check_timeout() const { return false; }

private:
  inline worker(const std::shared_ptr<work_item_queue> &queue)
      : _queue(queue) {}

  void work();

  std::shared_ptr<std::thread> _thread;
  std::shared_ptr<work_item_queue> _queue;
};
} // namespace threads
} // namespace s3

#endif
