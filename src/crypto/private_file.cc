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

#include "crypto/private_file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>

namespace s3 {
namespace crypto {

void PrivateFile::Open(const std::string &file, std::ofstream *f,
                       OpenMode mode) {
  std::ifstream test_open(file.c_str(), std::ios::in);

  if (test_open.good() && mode != OpenMode::OVERWRITE)
    throw std::runtime_error("file already exists");

  f->open(file.c_str(), std::ios::out | std::ios::trunc);

  if (!f->good())
    throw std::runtime_error("unable to open/create private file.");

  if (chmod(file.c_str(), S_IRUSR | S_IWUSR))
    throw std::runtime_error("failed to set permissions on private file.");
}

void PrivateFile::Open(const std::string &file, std::ifstream *f) {
  struct stat s;

  f->open(file.c_str(), std::ios::in);

  if (!f->good()) throw std::runtime_error("unable to open private file.");

  if (stat(file.c_str(), &s))
    throw std::runtime_error("unable to stat private file.");

  if ((s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRUSR | S_IWUSR))
    throw std::runtime_error(
        "private file must be readable/writeable only by owner.");
}

}  // namespace crypto
}  // namespace s3
