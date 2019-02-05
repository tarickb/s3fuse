/*
 * crypto/private_file.cc
 * -------------------------------------------------------------------------
 * "Private" file management (implementation).
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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>

#include "base/paths.h"
#include "crypto/private_file.h"

namespace s3 {
namespace crypto {

void private_file::open(const std::string &file_, std::ofstream *f,
                        open_mode mode) {
  std::string file = base::paths::transform(file_);
  std::ifstream test_open(file.c_str(), std::ios::in);

  if (test_open.good() && mode != OM_OVERWRITE)
    throw std::runtime_error("file already exists");

  f->open(file.c_str(), std::ios::out | std::ios::trunc);

  if (!f->good())
    throw std::runtime_error("unable to open/create private file.");

  if (chmod(file.c_str(), S_IRUSR | S_IWUSR))
    throw std::runtime_error("failed to set permissions on private file.");
}

void private_file::open(const std::string &file_, std::ifstream *f) {
  std::string file = base::paths::transform(file_);
  struct stat s;

  f->open(file.c_str(), std::ios::in);

  if (!f->good())
    throw std::runtime_error("unable to open private file.");

  if (stat(file.c_str(), &s))
    throw std::runtime_error("unable to stat private file.");

  if ((s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRUSR | S_IWUSR))
    throw std::runtime_error(
        "private file must be readable/writeable only by owner.");
}

} // namespace crypto
} // namespace s3
