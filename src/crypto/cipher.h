/*
 * crypto/cipher.h
 * -------------------------------------------------------------------------
 * Templated interface to cipher classes.
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

#ifndef S3_CRYPTO_CIPHER_H
#define S3_CRYPTO_CIPHER_H

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "crypto/encoder.h"

namespace s3 {
namespace crypto {
class SymmetricKey;

class Cipher {
 public:
  template <class CipherType>
  inline static std::vector<uint8_t> Encrypt(const SymmetricKey &key,
                                             const uint8_t *input,
                                             size_t size) {
    return CipherType::Encrypt(key, input, size);
  }

  template <class CipherType, class EncoderType>
  inline static std::string Encrypt(const SymmetricKey &key,
                                    const uint8_t *input, size_t size) {
    return Encoder::Encode<EncoderType>(CipherType::Encrypt(key, input, size));
  }

  template <class CipherType, class EncoderType>
  inline static std::string Encrypt(const SymmetricKey &key,
                                    const std::string &input) {
    return Encrypt<CipherType, EncoderType>(
        key, reinterpret_cast<const uint8_t *>(input.c_str()),
        input.size() + 1);
  }

  template <class CipherType>
  inline static std::vector<uint8_t> Decrypt(const SymmetricKey &key,
                                             const uint8_t *input,
                                             size_t size) {
    return CipherType::Decrypt(key, input, size);
  }

  template <class CipherType>
  inline static std::string DecryptAsString(const SymmetricKey &key,
                                            const uint8_t *input, size_t size) {
    const auto output = CipherType::Decrypt(key, input, size);
    if (output.empty())
      throw std::runtime_error("decrypt resulted in an empty string.");
    if (output.back() != '\0')
      throw std::runtime_error(
          "cannot decrypt to string if last byte is non-null");
    return std::string(reinterpret_cast<const char *>(&output[0]));
  }

  template <class CipherType, class EncoderType>
  inline static std::string DecryptAsString(const SymmetricKey &key,
                                            const std::string &input) {
    auto decoded = Encoder::Decode<EncoderType>(input);
    return DecryptAsString<CipherType>(key, &decoded[0], decoded.size());
  }
};
}  // namespace crypto
}  // namespace s3

#endif
