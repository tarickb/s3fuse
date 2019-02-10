/*
 * threads/worker.cc
 * -------------------------------------------------------------------------
 * Pool worker thread (implementation).
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

#include "threads/worker.h"

#include <memory>

#include "threads/work_item_queue.h"

namespace s3 {
namespace threads {

std::unique_ptr<Worker> Worker::Create(WorkItemQueue *queue) {
  return std::unique_ptr<Worker>(new Worker(queue));
}

Worker::Worker(WorkItemQueue *queue)
    : queue_(queue), thread_(&Worker::Work, this) {}

Worker::~Worker() { thread_.join(); }

void Worker::Work() {
  while (true) {
    auto item = queue_->GetNext();
    if (!item.valid()) break;
    item.Run({});
  }
}

}  // namespace threads
}  // namespace s3
