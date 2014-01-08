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

#include <stdexcept>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class symmetric_key;

    class aes_ctr_256
    {
    public:
      enum { BLOCK_LEN = AES_BLOCK_SIZE };

      enum { IV_LEN = BLOCK_LEN / 2 };
      enum { DEFAULT_KEY_LEN = 32 }; // 256 bits

      inline static void encrypt_with_byte_offset(const boost::shared_ptr<symmetric_key> &key, uint64_t offset, const uint8_t *in, size_t size, uint8_t *out)
      {
        if (offset % BLOCK_LEN)
          throw std::runtime_error("offset must be a multiple of BLOCK_LEN");

        crypt(key, offset / BLOCK_LEN, in, size, out);
      }

      inline static void decrypt_with_byte_offset(const boost::shared_ptr<symmetric_key> &key, uint64_t offset, const uint8_t *in, size_t size, uint8_t *out)
      {
        encrypt_with_byte_offset(key, offset, in, size, out);
      }

      inline static void encrypt_with_starting_block(const boost::shared_ptr<symmetric_key> &key, uint64_t starting_block, const uint8_t *in, size_t size, uint8_t *out)
      {
        crypt(key, starting_block, in, size, out);
      }

      inline static void decrypt_with_starting_block(const boost::shared_ptr<symmetric_key> &key, uint64_t starting_block, const uint8_t *in, size_t size, uint8_t *out)
      {
        encrypt_with_starting_block(key, starting_block, in, size, out);
      }

      inline static void encrypt(const boost::shared_ptr<symmetric_key> &key, const uint8_t *in, size_t size, uint8_t *out)
      {
        crypt(key, 0, in, size, out);
      }

      inline static void decrypt(const boost::shared_ptr<symmetric_key> &key, const uint8_t *in, size_t size, uint8_t *out)
      {
        encrypt(key, in, size, out);
      }

    private:
      static void crypt(const boost::shared_ptr<symmetric_key> &key, uint64_t starting_block, const uint8_t *in, size_t size, uint8_t *out);
    };
  }
}

#endif
