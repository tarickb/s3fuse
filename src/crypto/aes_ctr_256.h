/*
 * crypto/aes_ctr_256.h
 * -------------------------------------------------------------------------
 * AES in counter mode with a default key length of 256 bits.
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

#ifndef S3_CRYPTO_AES_CTR_256_H
#define S3_CRYPTO_AES_CTR_256_H

#include <stdint.h>

#include <openssl/aes.h>

#include <memory>
#include <stdexcept>

namespace s3 {
namespace crypto {
class SymmetricKey;

class AesCtr256 {
 public:
  static constexpr int BLOCK_LEN = AES_BLOCK_SIZE;

  static constexpr int IV_LEN = BLOCK_LEN / 2;
  static constexpr int DEFAULT_KEY_LEN = 32;  // 256 bits

  inline static void EncryptWithByteOffset(const SymmetricKey &key,
                                           uint64_t offset, const uint8_t *in,
                                           size_t size, uint8_t *out) {
    if (offset % BLOCK_LEN)
      throw std::runtime_error("offset must be a multiple of BLOCK_LEN");

    Crypt(key, offset / BLOCK_LEN, in, size, out);
  }

  inline static void DecryptWithByteOffset(const SymmetricKey &key,
                                           uint64_t offset, const uint8_t *in,
                                           size_t size, uint8_t *out) {
    EncryptWithByteOffset(key, offset, in, size, out);
  }

  inline static void EncryptWithStartingBlock(const SymmetricKey &key,
                                              uint64_t starting_block,
                                              const uint8_t *in, size_t size,
                                              uint8_t *out) {
    Crypt(key, starting_block, in, size, out);
  }

  inline static void DecryptWithStartingBlock(const SymmetricKey &key,
                                              uint64_t starting_block,
                                              const uint8_t *in, size_t size,
                                              uint8_t *out) {
    EncryptWithStartingBlock(key, starting_block, in, size, out);
  }

  inline static void Encrypt(const SymmetricKey &key, const uint8_t *in,
                             size_t size, uint8_t *out) {
    Crypt(key, 0, in, size, out);
  }

  inline static void Decrypt(const SymmetricKey &key, const uint8_t *in,
                             size_t size, uint8_t *out) {
    Encrypt(key, in, size, out);
  }

 private:
  static void Crypt(const SymmetricKey &key, uint64_t starting_block,
                    const uint8_t *in, size_t size, uint8_t *out);
};
}  // namespace crypto
}  // namespace s3

#endif
