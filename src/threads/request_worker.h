/*
 * threads/RequestWorker.h
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

#ifndef S3_THREADS_RequestWorker_H
#define S3_THREADS_RequestWorker_H

#include <functional>
#include <memory>
#include <thread>

#include "threads/work_item.h"

namespace s3 {
namespace base {
class Request;
}

namespace threads {
class WorkItemQueue;

class RequestWorker {
 public:
  static std::unique_ptr<RequestWorker> Create(WorkItemQueue *queue);

  ~RequestWorker();

 private:
  RequestWorker(WorkItemQueue *queue);

  void Work();

  std::unique_ptr<base::Request> request_;
  WorkItemQueue *queue_ = nullptr;
  std::thread thread_;
};
}  // namespace threads
}  // namespace s3

#endif
