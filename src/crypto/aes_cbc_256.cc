/*
 * crypto/aes_cbc_256.cc
 * -------------------------------------------------------------------------
 * AES in cipher block chaining mode (implementation).
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

#include "crypto/aes_cbc_256.h"

#include <openssl/evp.h>

#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {

std::vector<uint8_t> AesCbc256::Crypt(Mode mode, bool pad,
                                      const SymmetricKey &key,
                                      const uint8_t *in, size_t size) {
  std::vector<uint8_t> out;
  if (key.iv().size() != IV_LEN)
    throw std::runtime_error("iv length is not valid for aes_cbc_256");

  if (mode == Mode::ENCRYPT) {
    // allow for padding
    out.resize(size + BLOCK_LEN);
  } else {
    if (size % BLOCK_LEN)
      throw std::runtime_error(
          "input size must be a multiple of BLOCK_LEN "
          "when decrypting with aes_cbc_256");

    out.resize(size);
  }

  const EVP_CIPHER *cipher = nullptr;

  switch (key.key().size()) {
    case (128 / 8):
      cipher = EVP_aes_128_cbc();
      break;

    case (192 / 8):
      cipher = EVP_aes_192_cbc();
      break;

    case (256 / 8):
      cipher = EVP_aes_256_cbc();
      break;
  }

  if (!cipher) throw std::runtime_error("invalid key length for aes_cbc_256");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  try {
    int updated = 0, finalized = 0;

    if (mode == Mode::ENCRYPT) {
      if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key.key().get(),
                             key.iv().get()) == 0)
        throw std::runtime_error("EVP_EncryptInit_ex() failed in aes_cbc_256");

      if (!pad) EVP_CIPHER_CTX_set_padding(ctx, 0);

      if (!EVP_EncryptUpdate(ctx, &out[0], &updated, in, size))
        throw std::runtime_error("EVP_EncryptUpdate() failed in aes_cbc_256");

      if (!EVP_EncryptFinal_ex(ctx, &out[updated], &finalized))
        throw std::runtime_error("EVP_EncryptFinal_ex() failed in aes_cbc_256");
    } else {
      if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key.key().get(),
                             key.iv().get()) == 0)
        throw std::runtime_error("EVP_DecryptInit_ex() failed in aes_cbc_256");

      if (!pad) EVP_CIPHER_CTX_set_padding(ctx, 0);

      if (!EVP_DecryptUpdate(ctx, &out[0], &updated, in, size))
        throw std::runtime_error("EVP_DecryptUpdate() failed in aes_cbc_256");

      if (!EVP_DecryptFinal_ex(ctx, &out[updated], &finalized))
        throw std::runtime_error("EVP_DecryptFinal_ex() failed in aes_cbc_256");
    }

    out.resize(updated + finalized);

  } catch (...) {
    EVP_CIPHER_CTX_free(ctx);
    throw;
  }

  EVP_CIPHER_CTX_free(ctx);
  return out;
}

}  // namespace crypto
}  // namespace s3
