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

#include <memory>
#include <thread>

namespace s3 {
namespace threads {
class WorkItemQueue;

class Worker {
 public:
  static std::unique_ptr<Worker> Create(WorkItemQueue *queue);

  ~Worker();

 private:
  explicit Worker(WorkItemQueue *queue);

  void Work();

  WorkItemQueue *queue_ = nullptr;
  std::thread thread_;
};
}  // namespace threads
}  // namespace s3

#endif
