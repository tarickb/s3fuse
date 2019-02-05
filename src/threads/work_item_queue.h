/*
 * threads/work_item_queue.h
 * -------------------------------------------------------------------------
 * Pool work item queue.
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

#ifndef S3_THREADS_WORK_ITEM_QUEUE_H
#define S3_THREADS_WORK_ITEM_QUEUE_H

#include <deque>

#include <condition_variable>
#include <memory>
#include <thread>

#include "threads/work_item.h"

namespace s3 {
namespace threads {
class work_item_queue {
public:
  typedef std::shared_ptr<work_item_queue> ptr;

  inline work_item_queue() : _done(false) {}

  inline work_item get_next() {
    std::unique_lock<std::mutex> lock(_mutex);
    work_item item;

    while (!_done && _queue.empty())
      _condition.wait(lock);

    if (_done)
      return work_item(); // generates an invalid work item

    item = _queue.front();
    _queue.pop_front();

    return item;
  }

  inline void post(const work_item &item) {
    std::lock_guard<std::mutex> lock(_mutex);

    _queue.push_back(item);
    _condition.notify_all();
  }

  inline void abort() {
    std::lock_guard<std::mutex> lock(_mutex);

    _done = true;
    _condition.notify_all();
  }

private:
  std::mutex _mutex;
  std::condition_variable _condition;
  std::deque<work_item> _queue;
  bool _done;
};
} // namespace threads
} // namespace s3

#endif
