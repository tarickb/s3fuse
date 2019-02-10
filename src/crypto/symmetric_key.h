/*
 * crypto/symmetric_key.h
 * -------------------------------------------------------------------------
 * Contains a symmetric key and an initialization vector, and allows for
 * serialization and deserialization.
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

#ifndef S3_CRYPTO_SYMMETRIC_KEY_H
#define S3_CRYPTO_SYMMETRIC_KEY_H

#include <memory>
#include <string>

#include "crypto/buffer.h"

namespace s3 {
namespace crypto {
class SymmetricKey {
 public:
  template <class CipherType>
  inline static SymmetricKey Generate() {
    return Create(Buffer::Generate(CipherType::DEFAULT_KEY_LEN),
                  Buffer::Generate(CipherType::IV_LEN));
  }

  template <class CipherType>
  inline static SymmetricKey Generate(size_t key_len) {
    return Create(Buffer::Generate(key_len),
                  Buffer::Generate(CipherType::IV_LEN));
  }

  template <class CipherType>
  inline static SymmetricKey Generate(const Buffer &key) {
    return Create(key, Buffer::Generate(CipherType::IV_LEN));
  }

  inline static SymmetricKey FromString(const std::string &str) {
    size_t pos = str.find(':');
    if (pos == std::string::npos)
      throw std::runtime_error("malformed symmetric key string");
    return Create(Buffer::FromHexString(str.substr(0, pos)),
                  Buffer::FromHexString(str.substr(pos + 1)));
  }

  inline static SymmetricKey Create(const Buffer &key, const Buffer &iv) {
    return {key, iv};
  }

  inline static SymmetricKey Empty() {
    return {Buffer::Empty(), Buffer::Empty()};
  }

  inline const Buffer &key() const { return key_; }
  inline const Buffer &iv() const { return iv_; }

  inline operator bool() const { return key_ && iv_; }

  inline std::string ToString() const {
    return key_.ToHexString() + ":" + iv_.ToHexString();
  }

 private:
  inline SymmetricKey(const Buffer &key, const Buffer &iv)
      : key_(key), iv_(iv) {}

  Buffer key_, iv_;
};
}  // namespace crypto
}  // namespace s3

#endif
