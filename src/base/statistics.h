/*
 * base/statistics.h
 * -------------------------------------------------------------------------
 * Statistics collection and output.
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

#ifndef S3_BASE_STATISTICS_H
#define S3_BASE_STATISTICS_H

#include <stdarg.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "base/paths.h"
#include "base/static_list.h"

namespace s3 {
namespace base {
class Statistics {
 public:
  using WriterCallback = std::function<void(std::ostream *)>;
  using Writers = StaticList<WriterCallback>;

  static void Init(std::unique_ptr<std::ostream> output);
  static void Init(std::string file);

  static void Collect();
  static void Flush();
  static void Write(std::string id, const char *format, ...);
  static void Write(std::string id, std::string tag, const char *format, ...);
};
}  // namespace base
}  // namespace s3

#endif
