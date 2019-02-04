/*
 * base/curl_easy_handle.h
 * -------------------------------------------------------------------------
 * CURL easy handle wrapper
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

#ifndef S3_BASE_CURL_EASY_HANDLE_H
#define S3_BASE_CURL_EASY_HANDLE_H

#include <curl/curl.h>

namespace s3
{
  namespace base
  {
    class curl_easy_handle
    {
    public:
      curl_easy_handle();
      ~curl_easy_handle();

      curl_easy_handle(const curl_easy_handle &) = delete;
      curl_easy_handle & operator =(const curl_easy_handle &) = delete;

      inline operator const CURL * () const { return _handle; }
      inline operator CURL * () { return _handle; }

    private:
      CURL *_handle;
    };
  }
}

#endif
