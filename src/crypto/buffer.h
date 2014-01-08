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

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

#include "crypto/encoder.h"
#include "crypto/hex.h"

namespace s3
{
  namespace crypto
  {
    class buffer
    {
    public:
      typedef boost::shared_ptr<buffer> ptr;

      inline static ptr zero(size_t len)
      {
        ptr b(new buffer());

        b->_buf.resize(len);
        memset(&b->_buf[0], 0, len);

        return b;
      }

      inline static ptr generate(size_t len)
      {
        ptr b(new buffer());

        if (!len)
          throw std::runtime_error("cannot generate empty buffer");

        b->_buf.resize(len);

        if (RAND_bytes(&b->_buf[0], len) == 0)
          throw std::runtime_error("failed to generate random key");

        return b;
      }

      inline static ptr from_string(const std::string &in)
      {
        ptr b(new buffer());

        encoder::decode<hex>(in, &b->_buf);

        return b;
      }

      inline static ptr from_vector(const std::vector<uint8_t> &bytes)
      {
        ptr b(new buffer());

        b->_buf = bytes;

        return b;
      }

      inline const uint8_t * get() const { return &_buf[0]; }
      inline size_t size() const { return _buf.size(); }

      inline std::string to_string()
      {
        return encoder::encode<hex>(_buf);
      }

    private:
      inline buffer()
      {
      }

      std::vector<uint8_t> _buf;
    };
  }
}

#endif
