/*
 * crypto/hash_list.h
 * -------------------------------------------------------------------------
 * Templated hash generator that accepts input data in unordered chunks.
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

#ifndef S3_CRYPTO_HASH_LIST_H
#define S3_CRYPTO_HASH_LIST_H

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "crypto/encoder.h"
#include "crypto/hash.h"

namespace s3 {
namespace crypto {
template <class HashType>
class HashList {
 public:
  static const size_t CHUNK_SIZE = 128 * 1024;  // 128 KB

  inline explicit HashList(size_t total_size)
      : hashes_((total_size + CHUNK_SIZE - 1) / CHUNK_SIZE *
                HashType::HASH_LEN) {}

  // this is thread safe so long as no two threads try to update the same part
  inline void ComputeHash(size_t offset, const uint8_t *data, size_t size) {
    size_t remaining_size = size;

    if (offset % CHUNK_SIZE)
      throw std::runtime_error(
          "cannot compute hash if offset is not chunk-aligned");

    for (size_t o = 0; o < size; o += CHUNK_SIZE) {
      size_t chunk_size =
          (remaining_size > CHUNK_SIZE) ? CHUNK_SIZE : remaining_size;

      Hash::Compute<HashType>(
          data + o, chunk_size,
          &hashes_[(offset + o) / CHUNK_SIZE * HashType::HASH_LEN]);

      remaining_size -= chunk_size;
    }
  }

  template <class EncoderType>
  inline std::string GetRootHash() {
    uint8_t root_hash[HashType::HASH_LEN];
    Hash::Compute<HashType>(hashes_, root_hash);
    return Encoder::Encode<EncoderType>(root_hash, HashType::HASH_LEN);
  }

 private:
  std::vector<uint8_t> hashes_;
};
}  // namespace crypto
}  // namespace s3

#endif
