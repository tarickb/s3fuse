/*
 * base/logger.cc
 * -------------------------------------------------------------------------
 * Definitions for s3::base::logger static members and init() method.
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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include <limits>

namespace s3 {
namespace base {

namespace {
// log all messages unless instructed otherwise
int s_max_level = std::numeric_limits<int>::max();
Logger::Mode s_mode = Logger::Mode::SYSLOG;
}  // namespace

void Logger::Init(Mode mode, int max_level) {
  s_max_level = max_level;
  s_mode = mode;
  if (s_mode == Mode::SYSLOG) openlog(PACKAGE_NAME, 0, 0);
}

void Logger::Log(int level, const char *message, ...) {
  va_list args;

  if (level <= s_max_level) {
    // can't reuse va_list
    if (s_mode == Mode::SYSLOG) {
      va_start(args, message);
      vsyslog(level, message, args);
      va_end(args);
    } else if (s_mode == Mode::STDERR) {
      va_start(args, message);
      vfprintf(stderr, message, args);
      va_end(args);
    }
  }
}

}  // namespace base
}  // namespace s3
