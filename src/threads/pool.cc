/*
 * threads/pool.cc
 * -------------------------------------------------------------------------
 * Implements a pool of worker threads.
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

#include "threads/pool.h"

#include <list>
#include <map>
#include <memory>
#include <string>

#include "threads/request_worker.h"
#include "threads/work_item_queue.h"
#include "threads/worker.h"

namespace s3 {
namespace threads {

namespace {
constexpr int NUM_THREADS_PER_POOL = 8;

class _Pool {
 public:
  virtual ~_Pool() = default;

  virtual void Post(WorkItem::WorkerFunction fn,
                    WorkItem::CallbackFunction cb) = 0;
};

template <class WorkerType>
class _PoolImpl : public _Pool {
 public:
  explicit _PoolImpl(const std::string &id) : id_(id) {
    for (int i = 0; i < NUM_THREADS_PER_POOL; i++)
      workers_.push_back(WorkerType::Create(&queue_));
  }

  ~_PoolImpl() {
    queue_.Abort();
    workers_.clear();
  }

  virtual void Post(WorkItem::WorkerFunction fn,
                    WorkItem::CallbackFunction cb) {
    queue_.Post({fn, cb});
  }

 private:
  std::string id_;
  WorkItemQueue queue_;
  std::list<std::unique_ptr<WorkerType>> workers_;
};

std::map<PoolId, std::unique_ptr<_Pool>> s_pools;
}  // namespace

void Pool::Init() {
  s_pools[PoolId::PR_0].reset(new _PoolImpl<Worker>("PR_0"));
  s_pools[PoolId::PR_REQ_0].reset(new _PoolImpl<RequestWorker>("PR_REQ_0"));
  s_pools[PoolId::PR_REQ_1].reset(new _PoolImpl<RequestWorker>("PR_REQ_1"));
}

void Pool::Terminate() { s_pools.clear(); }

void Pool::Post(PoolId p, WorkItem::WorkerFunction fn,
                WorkItem::CallbackFunction cb) {
  s_pools[p]->Post(fn, cb);
}

}  // namespace threads
}  // namespace s3
