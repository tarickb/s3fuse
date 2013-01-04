/*
 * base/request_hook.h
 * -------------------------------------------------------------------------
 * Defines a hook interface for the request class.
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

#ifndef S3_BASE_REQUEST_HOOK_H
#define S3_BASE_REQUEST_HOOK_H

#include <string>

namespace s3
{
  namespace base
  {
    class request;

    class request_hook
    {
    public:
      virtual ~request_hook() { }

      virtual std::string adjust_url(const std::string &url) = 0;
      virtual void pre_run(request *req, int iter) = 0;
      virtual bool should_retry(request *req, int iter) = 0;
    };
  }
}

#endif
