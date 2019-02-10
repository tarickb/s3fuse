/*
 * threads/request_worker.cc
 * -------------------------------------------------------------------------
 * Pool worker thread with attached request (implementation).
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

#include "threads/request_worker.h"

#include <atomic>
#include <iomanip>

#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/timer.h"
#include "threads/async_handle.h"
#include "threads/work_item_queue.h"

namespace s3 {
namespace threads {

namespace {
double s_total_req_time = 0.0;
double s_total_fn_time = 0.0;

std::mutex s_stats_mutex;
std::atomic_int s_reposted_items(0);

void StatsWriter(std::ostream *o) {
  o->setf(std::ostream::fixed);

  *o << "thread pool request workers:\n"
        "  total request time: "
     << std::setprecision(3) << s_total_req_time
     << " s\n"
        "  total function time: "
     << s_total_fn_time
     << " s\n"
        "  request wait: "
     << std::setprecision(2) << s_total_req_time / s_total_fn_time * 100.0
     << " %\n"
        "  reposted items: "
     << s_reposted_items << "\n";
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

std::unique_ptr<RequestWorker> RequestWorker::Create(WorkItemQueue *queue) {
  return std::unique_ptr<RequestWorker>(new RequestWorker(queue));
}

RequestWorker::RequestWorker(WorkItemQueue *queue)
    : request_(base::RequestFactory::New()),
      queue_(queue),
      thread_(&RequestWorker::Work, this) {}

RequestWorker::~RequestWorker() { thread_.join(); }

void RequestWorker::Work() {
  double time_in_function = 0.0, time_in_request = 0.0;

  while (true) {
    auto item = queue_->GetNext();
    if (!item.valid()) break;

    double start_time, end_time;

    start_time = base::Timer::GetCurrentTime();
    request_->ResetCurrentRunTime();

    item.Run(request_.get());

    end_time = base::Timer::GetCurrentTime();
    time_in_function += end_time - start_time;
    time_in_request += request_->current_run_time();
  }

  if (time_in_function > 0.0) {
    std::lock_guard<std::mutex> lock(s_stats_mutex);

    s_total_req_time += time_in_request;
    s_total_fn_time += time_in_function;
  }
}

}  // namespace threads
}  // namespace s3
