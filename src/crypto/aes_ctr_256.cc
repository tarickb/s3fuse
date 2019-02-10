/*
 * crypto/aes_ctr_256.cc
 * -------------------------------------------------------------------------
 * AES in counter mode (implementation).
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

#include "crypto/aes_ctr_256.h"

#include <openssl/modes.h>
#include <string.h>

#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {

void AesCtr256::Crypt(const SymmetricKey &key, uint64_t starting_block,
                      const uint8_t *in, size_t size, uint8_t *out) {
  uint8_t iv[BLOCK_LEN];

  if (key.iv().size() != IV_LEN)
    throw std::runtime_error("iv length is not valid for aes_ctr_256");

  starting_block = __builtin_bswap64(starting_block);

  memcpy(iv, key.iv().get(), IV_LEN);
  memcpy(iv + IV_LEN, &starting_block, sizeof(starting_block));

  AES_KEY aes_key;
  uint8_t ecount_buf[BLOCK_LEN];
  unsigned int num = 0;

  memset(ecount_buf, 0, sizeof(ecount_buf));

  if (AES_set_encrypt_key(key.key().get(), key.key().size() * 8 /* in bits */,
                          &aes_key) != 0)
    throw std::runtime_error("failed to set encryption key for aes_ctr_256");

  CRYPTO_ctr128_encrypt(in, out, size, &aes_key, iv, ecount_buf, &num,
                        reinterpret_cast<block128_f>(AES_encrypt));
}

}  // namespace crypto
}  // namespace s3
