/*
 * base/paths.cc
 * -------------------------------------------------------------------------
 * Path transformations.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#include "base/paths.h"

#include <stdlib.h>

namespace s3 {
namespace base {

std::string Paths::Transform(const std::string &path) {
  if (!path.empty() && path[0] == '~')
    return std::string(getenv("HOME")) + (path.c_str() + 1);
  else
    return path;
}

}  // namespace base
}  // namespace s3
