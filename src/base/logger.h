/*
 * base/logger.h
 * -------------------------------------------------------------------------
 * Provides logging to stderr and syslog.
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

#ifndef S3_BASE_LOGGER_H
#define S3_BASE_LOGGER_H

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <syslog.h>

namespace s3 {
namespace base {
class Logger {
 public:
  enum class Mode { NONE, STDERR, SYSLOG };

  static void Init(Mode mode, int max_level);
  static void Log(int level, const char *message, ...);
};
}  // namespace base
}  // namespace s3

// "##" between "," and "__VA_ARGS__" removes the comma if no arguments are
// provided
#define S3_LOG(level, fn, str, ...)                           \
  do {                                                        \
    s3::base::Logger::Log(level, fn ": " str, ##__VA_ARGS__); \
  } while (0)

#endif
