/*
 * threads/async_handle.h
 * -------------------------------------------------------------------------
 * Asynchronous event handle.
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

#ifndef S3_THREADS_ASYNC_HANDLE_H
#define S3_THREADS_ASYNC_HANDLE_H

#include <condition_variable>
#include <mutex>

namespace s3 {
namespace threads {
class AsyncHandle {
 public:
  inline AsyncHandle() = default;
  inline ~AsyncHandle() = default;

  inline void Complete(int return_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    return_code_ = return_code;
    done_ = true;
    condition_.notify_all();
  }

  inline int Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done_) condition_.wait(lock);
    return return_code_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;

  int return_code_ = 0;
  bool done_ = false;
};
}  // namespace threads
}  // namespace s3

#endif
