/*
 * crypto/md5.h
 * -------------------------------------------------------------------------
 * MD5 hasher.
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

#ifndef S3_CRYPTO_MD5_H
#define S3_CRYPTO_MD5_H

#include <cstdint>
#include <string>

namespace s3 {
namespace crypto {
class Hash;

class Md5 {
 public:
  static constexpr int HASH_LEN = 128 / 8;

  inline static bool IsValidQuotedHexHash(const std::string &hash) {
    if (hash.size() != 2 * HASH_LEN + 2)  // *2 for hex encoding, +2 for quotes
      return false;

    if (hash.front() != '"' || hash.back() != '"') return false;

    for (size_t i = 1; i < hash.size() - 1; i++)
      if ((hash[i] < '0' || hash[i] > '9') &&
          (hash[i] < 'a' || hash[i] > 'f') && (hash[i] < 'A' || hash[i] > 'F'))
        return false;

    return true;
  }

 private:
  friend class Hash;

  static void Compute(const uint8_t *input, size_t size, uint8_t *hash);
  static void Compute(int fd, uint8_t *hash);
};
}  // namespace crypto
}  // namespace s3

#endif
