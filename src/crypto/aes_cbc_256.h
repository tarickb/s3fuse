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

#include <stdint.h>

#include <openssl/aes.h>

#include <memory>
#include <stdexcept>
#include <vector>

namespace s3 {
namespace crypto {
class cipher;
class symmetric_key;

class aes_cbc_256 {
public:
  enum { BLOCK_LEN = AES_BLOCK_SIZE };

  enum { IV_LEN = BLOCK_LEN };
  enum { DEFAULT_KEY_LEN = 32 }; // 256 bits

protected:
  enum crypt_mode {
    M_ENCRYPT = 0x1,
    M_DECRYPT = 0x2,

    M_NO_PAD = 0x1000
  };

  static void crypt(int mode, const std::shared_ptr<symmetric_key> &key,
                    const uint8_t *in, size_t size, std::vector<uint8_t> *out);
};

class aes_cbc_256_with_pkcs : public aes_cbc_256 {
private:
  friend class cipher; // for encrypt() and decrypt()

  inline static void encrypt(const std::shared_ptr<symmetric_key> &key,
                             const uint8_t *in, size_t size,
                             std::vector<uint8_t> *out) {
    crypt(M_ENCRYPT, key, in, size, out);
  }

  inline static void decrypt(const std::shared_ptr<symmetric_key> &key,
                             const uint8_t *in, size_t size,
                             std::vector<uint8_t> *out) {
    crypt(M_DECRYPT, key, in, size, out);
  }
};

class aes_cbc_256_no_padding : public aes_cbc_256 {
private:
  friend class cipher; // for encrypt() and decrypt()

  inline static void encrypt(const std::shared_ptr<symmetric_key> &key,
                             const uint8_t *in, size_t size,
                             std::vector<uint8_t> *out) {
    crypt(M_ENCRYPT | M_NO_PAD, key, in, size, out);
  }

  inline static void decrypt(const std::shared_ptr<symmetric_key> &key,
                             const uint8_t *in, size_t size,
                             std::vector<uint8_t> *out) {
    crypt(M_DECRYPT | M_NO_PAD, key, in, size, out);
  }
};
} // namespace crypto
} // namespace s3

#endif
