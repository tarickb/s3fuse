/*
 * crypto/private_file.h
 * -------------------------------------------------------------------------
 * Opens "private" files (i.e., where only the owner has read access).
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

#ifndef S3_CRYPTO_PRIVATE_FILE_H
#define S3_CRYPTO_PRIVATE_FILE_H

#include <fstream>
#include <string>

namespace s3 {
namespace crypto {
class PrivateFile {
 public:
  enum class OpenMode { DEFAULT, OVERWRITE };

  static void Open(const std::string &file, std::ifstream *f);
  static void Open(const std::string &file, std::ofstream *f,
                   OpenMode mode = OpenMode::DEFAULT);
};
}  // namespace crypto
}  // namespace s3

#endif
