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

#include <iostream>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "threads/pool.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace threads
  {
    // TODO: rewrite this so that it uses just a part ID (so as to not copy vectors and all that shit)

    template <class T>
    class parallel_work_queue
    {
    public:
      typedef boost::function2<int, const boost::shared_ptr<base::request> &, T *> process_part_fn;
      typedef boost::function2<int, const boost::shared_ptr<base::request> &, T *> retry_part_fn;

      inline parallel_work_queue(
        const std::vector<T> &parts,
        const process_part_fn &on_process_part,
        const retry_part_fn &on_retry_part,
        int max_retries = -1,
        int max_parts_in_progress = -1)
        : _on_process_part(on_process_part),
          _on_retry_part(on_retry_part)
      {
        _parts.resize(parts.size());

        for (size_t i = 0; i < parts.size(); i++) {
          _parts[i].part = parts[i];
          _parts[i].id = i;
        }

        _max_retries = (max_retries == -1) ? base::config::get_max_transfer_retries() : max_retries;
        _max_parts_in_progress = (max_parts_in_progress == -1) ? base::config::get_max_parts_in_progress() : max_parts_in_progress;
      }

      int process()
      {
        size_t last_part = 0;
        std::list<process_part *> parts_in_progress;
        int r = 0;

        for (last_part = 0; last_part < std::min(_max_parts_in_progress, _parts.size()); last_part++) {
          process_part *part = &_parts[last_part];

          part->handle = threads::pool::post(
            threads::PR_REQ_1, 
            bind(_on_process_part, _1, &part->part),
            0 /* don't retry on timeout since we handle that here */);

          parts_in_progress.push_back(part);
        }

        while (!parts_in_progress.empty()) {
          process_part *part = parts_in_progress.front();
          int part_r;

          parts_in_progress.pop_front();
          part_r = part->handle->wait();

          if (part_r) {
            S3_LOG(LOG_DEBUG, "parallel_work_queue::process", "part %i returned status %i.\n", part->id, part_r);

            if ((part_r == -EAGAIN || part_r == -ETIMEDOUT) && part->retry_count < _max_retries) {
              part->handle = threads::pool::post(
                threads::PR_REQ_1, 
                bind(_on_retry_part, _1, &part->part),
                0);

              part->retry_count++;

              parts_in_progress.push_back(part);
            } else {
              if (r == 0) // only save the first non-successful return code
                r = part_r;
            }
          }

          // keep collecting parts until we have nothing left pending
          // if one part fails, keep going but stop posting new parts

          if (r == 0 && last_part < _parts.size()) {
            part = &_parts[last_part++];

            part->handle = threads::pool::post(
              threads::PR_REQ_1, 
              bind(_on_process_part, _1, &part->part),
              0);

            parts_in_progress.push_back(part);
          }
        }

        return r;
      }

    private:
      struct process_part
      {
        int id;
        int retry_count;
        threads::wait_async_handle::ptr handle;

        T part;

        inline process_part()
          : id(-1),
            retry_count(0)
        {
        }
      };

      std::vector<process_part> _parts;

      process_part_fn _on_process_part;
      retry_part_fn _on_retry_part;

      int _max_retries;
      size_t _max_parts_in_progress;
    };
  }
}

#endif
