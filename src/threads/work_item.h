/*
 * threads/work_item.h
 * -------------------------------------------------------------------------
 * Pool work item.
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

#ifndef S3_THREADS_WORK_ITEM_H
#define S3_THREADS_WORK_ITEM_H

#include <functional>
#include <memory>

namespace s3 {
namespace base {
class Request;
}

namespace threads {
class WorkItem {
 public:
  using WorkerFunction = std::function<int(base::Request *)>;
  using CallbackFunction = std::function<void(int)>;

  inline WorkItem() = default;

  inline WorkItem(WorkerFunction function, CallbackFunction on_completion)
      : valid_(true),
        function_(std::move(function)),
        on_completion_(std::move(on_completion)) {}

  inline bool valid() const { return valid_; }

  void Run(base::Request *req);

 private:
  bool valid_ = false;
  WorkerFunction function_;
  CallbackFunction on_completion_;
};
}  // namespace threads
}  // namespace s3

#endif
