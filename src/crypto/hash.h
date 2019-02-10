/*
 * crypto/hash.h
 * -------------------------------------------------------------------------
 * Templated interface to hash classes.
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

#ifndef S3_CRYPTO_HASH_H
#define S3_CRYPTO_HASH_H

#include <stdint.h>

#include <string>
#include <vector>

#include "crypto/encoder.h"

namespace s3 {
namespace crypto {
class Hash {
 public:
  template <class HashType>
  inline static void Compute(const uint8_t *input, size_t size, uint8_t *hash) {
    HashType::Compute(input, size, hash);
  }

  template <class HashType>
  inline static void Compute(const char *input, size_t size, uint8_t *hash) {
    HashType::Compute(reinterpret_cast<const uint8_t *>(input), size, hash);
  }

  template <class HashType>
  inline static void Compute(const std::string &input, uint8_t *hash) {
    Compute<HashType>(input.c_str(), input.size() + 1, hash);
  }

  template <class HashType>
  inline static void Compute(const std::vector<uint8_t> &input, uint8_t *hash) {
    HashType::Compute(&input[0], input.size(), hash);
  }

  template <class HashType>
  inline static void Compute(const std::vector<char> &input, uint8_t *hash) {
    Compute<HashType>(&input[0], input.size(), hash);
  }

  template <class HashType>
  inline static void Compute(int fd, uint8_t *hash) {
    HashType::Compute(fd, hash);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(const uint8_t *input, size_t size) {
    uint8_t hash[HashType::HASH_LEN];
    HashType::Compute(input, size, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(const std::string &input) {
    uint8_t hash[HashType::HASH_LEN];
    Compute<HashType>(input, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(const char *input, size_t size) {
    uint8_t hash[HashType::HASH_LEN];
    Compute<HashType>(input, size, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(const std::vector<uint8_t> &input) {
    uint8_t hash[HashType::HASH_LEN];
    Compute<HashType>(input, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(const std::vector<char> &input) {
    uint8_t hash[HashType::HASH_LEN];
    Compute<HashType>(input, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }

  template <class HashType, class EncoderType>
  inline static std::string Compute(int fd) {
    uint8_t hash[HashType::HASH_LEN];
    Compute<HashType>(fd, hash);
    return Encoder::Encode<EncoderType>(hash, HashType::HASH_LEN);
  }
};
}  // namespace crypto
}  // namespace s3

#endif
