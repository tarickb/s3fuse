/*
 * base/timer.h
 * -------------------------------------------------------------------------
 * Various time-related functions.
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

#ifndef S3_BASE_TIMER_H
#define S3_BASE_TIMER_H

#include <sys/time.h>

#include <string>

namespace s3
{
  namespace base
  {
    class timer
    {
    public:
      inline static double get_current_time()
      {
        timeval t;

        gettimeofday(&t, NULL);

        return double(t.tv_sec) + double(t.tv_usec) / 1.0e6;
      }

      inline static std::string get_http_time()
      {
        time_t sys_time;
        tm gm_time;
        char time_str[128];

        time(&sys_time);
        gmtime_r(&sys_time, &gm_time);
        strftime(time_str, 128, "%a, %d %b %Y %H:%M:%S GMT", &gm_time);

        return time_str;
      }
    };
  }
}

#endif
