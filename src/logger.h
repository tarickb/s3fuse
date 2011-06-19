/*
 * logger.h
 * -------------------------------------------------------------------------
 * Provides logging to stderr and syslog.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#ifndef S3_LOGGER_H
#define S3_LOGGER_H

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

namespace s3
{
  class logger
  {
  public:
    static void init(int max_level);

    inline static void log(int level, const char *message, ...)
    {
      va_list args;

      if (level <= s_max_level) {
        va_start(args, message);
        vfprintf(stderr, message, args);
        va_end(args);
      }

      // can't reuse va_list

      va_start(args, message);
      vsyslog(level, message, args);
      va_end(args);
    }

  private:
    static int s_max_level;
  };
}

// "##" between "," and "__VA_ARGS__" removes the comma if no arguments are provided
#define S3_LOG(level, fn, str, ...) do { s3::logger::log(level, fn ": " str, ## __VA_ARGS__); } while (0)

#endif
