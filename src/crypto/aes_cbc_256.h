/*
 * crypto/aes_cbc_256.h
 * -------------------------------------------------------------------------
 * AES in cipher block chaining mode with a default key length of 256 bits.
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

#ifndef S3_CRYPTO_AES_CBC_256_H
#define S3_CRYPTO_AES_CBC_256_H

#include <openssl/aes.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace s3 {
namespace crypto {
class Cipher;
class SymmetricKey;

class AesCbc256 {
 public:
  static constexpr int BLOCK_LEN = AES_BLOCK_SIZE;

  static constexpr int IV_LEN = BLOCK_LEN;
  static constexpr int DEFAULT_KEY_LEN = 32;  // 256 bits

 protected:
  enum class Mode { ENCRYPT, DECRYPT };

  static std::vector<uint8_t> Crypt(Mode mode, bool pad,
                                    const SymmetricKey &key, const uint8_t *in,
                                    size_t size);
};

class AesCbc256WithPkcs : public AesCbc256 {
 private:
  friend class Cipher;  // for Encrypt() and Decrypt()

  inline static std::vector<uint8_t> Encrypt(const SymmetricKey &key,
                                             const uint8_t *in, size_t size) {
    return Crypt(Mode::ENCRYPT, true, key, in, size);
  }

  inline static std::vector<uint8_t> Decrypt(const SymmetricKey &key,
                                             const uint8_t *in, size_t size) {
    return Crypt(Mode::DECRYPT, true, key, in, size);
  }
};

class AesCbc256NoPadding : public AesCbc256 {
 private:
  friend class Cipher;  // for Encrypt() and Decrypt()

  inline static std::vector<uint8_t> Encrypt(const SymmetricKey &key,
                                             const uint8_t *in, size_t size) {
    return Crypt(Mode::ENCRYPT, false, key, in, size);
  }

  inline static std::vector<uint8_t> Decrypt(const SymmetricKey &key,
                                             const uint8_t *in, size_t size) {
    return Crypt(Mode::DECRYPT, false, key, in, size);
  }
};
}  // namespace crypto
}  // namespace s3

#endif
