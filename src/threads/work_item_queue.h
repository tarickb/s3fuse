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
#include <thread>

#include "threads/work_item.h"

namespace s3 {
namespace threads {
class WorkItemQueue {
 public:
  inline WorkItemQueue() = default;

  inline WorkItem GetNext() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done_ && queue_.empty()) condition_.wait(lock);
    if (done_) return {};  // generates an invalid work item
    auto item = queue_.front();
    queue_.pop_front();
    return item;
  }

  inline void Post(const WorkItem &item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(item);
    condition_.notify_all();
  }

  inline void Abort() {
    std::lock_guard<std::mutex> lock(mutex_);
    done_ = true;
    condition_.notify_all();
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<WorkItem> queue_;
  bool done_ = false;
};
}  // namespace threads
}  // namespace s3

#endif
