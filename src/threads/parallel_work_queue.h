/*
 * threads/parallel_work_queue.h
 * -------------------------------------------------------------------------
 * Work queue with parallel execution.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#ifndef S3_THREADS_PARALLEL_WORK_QUEUE_H
#define S3_THREADS_PARALLEL_WORK_QUEUE_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "threads/pool.h"

namespace s3 {
namespace base {
class Request;
}

namespace threads {
template <class Part>
class ParallelWorkQueue {
 public:
  using ProcessPartCallback = std::function<int(base::Request *, Part *)>;
  using RetryPartCallback = std::function<int(base::Request *, Part *)>;

  template <class Iterator>
  inline ParallelWorkQueue(Iterator begin, Iterator end,
                           const ProcessPartCallback &on_process_part,
                           const RetryPartCallback &on_retry_part,
                           int max_retries = -1, int max_parts_in_progress = -1)
      : on_process_part_(on_process_part), on_retry_part_(on_retry_part) {
    size_t id = 0;

    for (Iterator iter = begin; iter != end; ++iter) {
      PartInProgress p;
      p.part = &(*iter);
      p.id = id++;
      parts_.push_back(std::move(p));
    }

    max_retries_ = (max_retries == -1) ? base::Config::max_transfer_retries()
                                       : max_retries;
    max_parts_in_progress_ = (max_parts_in_progress == -1)
                                 ? base::Config::max_parts_in_progress()
                                 : max_parts_in_progress;
  }

  int Process() {
    size_t last_part = 0;
    std::list<PartInProgress *> parts_in_progress;
    int r = 0;

    for (last_part = 0;
         last_part < std::min(max_parts_in_progress_, parts_.size());
         last_part++) {
      PartInProgress *part = &parts_[last_part];

      part->handle = threads::Pool::Post(
          PoolId::PR_REQ_1,
          std::bind(on_process_part_, std::placeholders::_1, part->part));

      parts_in_progress.push_back(part);
    }

    while (!parts_in_progress.empty()) {
      PartInProgress *part = parts_in_progress.front();
      parts_in_progress.pop_front();
      int part_r = part->handle->Wait();

      if (part_r) {
        S3_LOG(LOG_DEBUG, "ParallelWorkQueue::Process",
               "part %i returned status %i.\n", part->id, part_r);

        if ((part_r == -EAGAIN || part_r == -ETIMEDOUT) &&
            part->retry_count < max_retries_) {
          part->handle = threads::Pool::Post(
              PoolId::PR_REQ_1,
              std::bind(on_retry_part_, std::placeholders::_1, part->part));

          part->retry_count++;

          parts_in_progress.push_back(part);
        } else {
          if (r == 0)  // only save the first non-successful return code
            r = part_r;
        }
      }

      // keep collecting parts until we have nothing left pending
      // if one part fails, keep going but stop posting new parts

      if (r == 0 && last_part < parts_.size()) {
        part = &parts_[last_part++];

        part->handle = threads::Pool::Post(
            PoolId::PR_REQ_1,
            std::bind(on_process_part_, std::placeholders::_1, part->part));

        parts_in_progress.push_back(part);
      }
    }

    return r;
  }

 private:
  struct PartInProgress {
    Part *part = nullptr;
    int id = -1;
    int retry_count = -1;
    std::unique_ptr<AsyncHandle> handle;
  };

  std::vector<PartInProgress> parts_;

  ProcessPartCallback on_process_part_;
  RetryPartCallback on_retry_part_;

  int max_retries_;
  size_t max_parts_in_progress_;
};
}  // namespace threads
}  // namespace s3

#endif
