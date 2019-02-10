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

namespace s3 {
namespace threads {
enum class PoolId { PR_0, PR_REQ_0, PR_REQ_1 };

class Pool {
 public:
  static void Init();
  static void Terminate();

  // This does the real work.
  static void Post(PoolId p, WorkItem::WorkerFunction fn,
                   WorkItem::CallbackFunction cb);

  // Convenience wrappers around Post() above.
  inline static std::unique_ptr<AsyncHandle> Post(PoolId p,
                                                  WorkItem::WorkerFunction fn) {
    std::unique_ptr<AsyncHandle> ah(new AsyncHandle());
    auto *const ah_raw = ah.get();
    auto complete = [ah_raw](int r) { ah_raw->Complete(r); };
    Post(p, fn, complete);
    return ah;
  }

  inline static int Call(PoolId p, WorkItem::WorkerFunction fn) {
    return Post(p, fn)->Wait();
  }

  inline static void CallAsync(PoolId p, WorkItem::WorkerFunction fn) {
    Post(p, fn, {});
  }
};
}  // namespace threads
}  // namespace s3

#endif
