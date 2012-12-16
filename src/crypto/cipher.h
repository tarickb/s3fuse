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

#include <stdint.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "crypto/encoder.h"

namespace s3
{
  namespace crypto
  {
    class symmetric_key;

    class cipher
    {
    public:
      template <class cipher_type>
      inline static void encrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size, 
        std::vector<uint8_t> *output)
      {
        cipher_type::encrypt(key, input, size, output);
      }

      template <class cipher_type, class encoder_type>
      inline static std::string encrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size)
      {
        std::vector<uint8_t> output;

        cipher_type::encrypt(key, input, size, &output);

        return encoder::encode<encoder_type>(output);
      }

      template <class cipher_type, class encoder_type>
      inline static std::string encrypt(
        const boost::shared_ptr<symmetric_key> &key,
        const std::string &input)
      {
        return encrypt<cipher_type, encoder_type>(key, reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
      }

      template <class cipher_type>
      inline static void decrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size, 
        std::vector<uint8_t> *output)
      {
        cipher_type::decrypt(key, input, size, output);
      }

      template <class cipher_type>
      inline static std::string decrypt(
        const boost::shared_ptr<symmetric_key> &key,
        const uint8_t *input,
        size_t size)
      {
        std::vector<uint8_t> output;

        cipher_type::decrypt(key, input, size, &output);

        if (output[output.size() - 1] != '\0')
          throw std::runtime_error("cannot decrypt to string if last byte is non-null");

        return std::string(reinterpret_cast<char *>(&output[0]));
      }

      template <class cipher_type, class encoder_type>
      inline static std::string decrypt(
        const boost::shared_ptr<symmetric_key> &key,
        const std::string &input_)
      {
        std::vector<uint8_t> input;

        encoder::decode<encoder_type>(input_, &input);

        return decrypt<cipher_type>(key, &input[0], input.size());
      }
    };
  }
}

#endif
