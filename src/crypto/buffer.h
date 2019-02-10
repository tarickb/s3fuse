/*
 * crypto/buffer.h
 * -------------------------------------------------------------------------
 * Generic, serializable crypto buffer.
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

#ifndef S3_CRYPTO_BUFFER_H
#define S3_CRYPTO_BUFFER_H

#include <string.h>

#include <openssl/rand.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "crypto/encoder.h"
#include "crypto/hex.h"

namespace s3 {
namespace crypto {
class Buffer {
 public:
  inline static Buffer Empty() { return Buffer(std::vector<uint8_t>()); }

  inline static Buffer Zero(size_t len) {
    std::vector<uint8_t> zero(len, 0);
    return Buffer(zero);
  }

  inline static Buffer Generate(size_t len) {
    if (!len) throw std::runtime_error("cannot generate empty buffer");
    std::vector<uint8_t> random(len);
    if (RAND_bytes(&random[0], len) == 0)
      throw std::runtime_error("failed to generate random key");
    return Buffer(random);
  }

  inline static Buffer FromHexString(const std::string &in) {
    return Buffer(Encoder::Decode<Hex>(in));
  }

  inline static Buffer FromVector(const std::vector<uint8_t> &bytes) {
    return Buffer(bytes);
  }

  inline const uint8_t *get() const { return &buf_[0]; }
  inline size_t size() const { return buf_.size(); }

  inline operator bool() const { return !buf_.empty(); }

  inline std::string ToHexString() const { return Encoder::Encode<Hex>(buf_); }

 private:
  inline explicit Buffer(std::vector<uint8_t> buf) : buf_(buf) {}

  std::vector<uint8_t> buf_;
};
}  // namespace crypto
}  // namespace s3

#endif
