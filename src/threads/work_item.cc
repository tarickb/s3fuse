/*
 * threads/work_item.cc
 * -------------------------------------------------------------------------
 * Pool work item (implementation).
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

#include "threads/work_item.h"

#include "base/logger.h"

namespace s3 {
namespace threads {

void WorkItem::Run(base::Request *req) {
  int r;
  try {
    r = function_(req);
  } catch (const std::exception &e) {
    S3_LOG(LOG_WARNING, "WorkItem::Run", "caught exception: %s\n", e.what());
    r = -ECANCELED;
  } catch (...) {
    S3_LOG(LOG_WARNING, "WorkItem::Run", "caught unknown exception.\n");
    r = -ECANCELED;
  }
  on_completion_(r);
}

}  // namespace threads
}  // namespace s3
