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

#include "base/logger.h"
#include "threads/async_handle.h"
#include "threads/work_item_queue.h"
#include "threads/worker.h"

using boost::shared_ptr;

using s3::base::request;
using s3::threads::worker;

void worker::work()
{
  shared_ptr<request> null_req;

  while (true) {
    work_item item;
    int r;

    item = _queue->get_next();

    if (!item.is_valid())
      break;

    try {
      r = item.get_function()(null_req);

    } catch (const std::exception &e) {
      S3_LOG(LOG_WARNING, "worker::work", "caught exception: %s\n", e.what());
      r = -ECANCELED;

    } catch (...) {
      S3_LOG(LOG_WARNING, "worker::work", "caught unknown exception.\n");
      r = -ECANCELED;
    }

    item.get_ah()->complete(r);
  }

  // the boost::thread in _thread holds a shared_ptr to this, and will keep it from being destructed
  _thread.reset();
}
