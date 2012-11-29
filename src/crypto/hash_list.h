/*
 * crypto/hash_list.h
 * -------------------------------------------------------------------------
 * Templated hash generator that accepts input data in unordered chunks.
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

#ifndef S3_CRYPTO_HASH_LIST_H
#define S3_CRYPTO_HASH_LIST_H

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

#include "crypto/encoder.h"
#include "crypto/hash.h"

namespace s3
{
  namespace crypto
  {
    template <class hash_type>
    class hash_list
    {
    public:
      typedef boost::shared_ptr<hash_list<hash_type> > ptr;

      static const size_t CHUNK_SIZE = 128 * 1024; // 128 KB

      inline hash_list(size_t total_size)
        : _hashes((total_size + CHUNK_SIZE - 1) / CHUNK_SIZE * hash_type::HASH_LEN)
      {
      }

      // this is thread safe so long as no two threads try to update the same part
      inline void compute_hash(size_t offset, const uint8_t *data, size_t size)
      {
        size_t remaining_size = size;

        if (offset % CHUNK_SIZE)
          throw std::runtime_error("cannot compute hash if offset is not chunk-aligned");

        for (size_t o = 0; o < size; o += CHUNK_SIZE) {
          size_t chunk_size = (remaining_size > CHUNK_SIZE) ? CHUNK_SIZE : remaining_size;

          hash::compute<hash_type>(
            data + o,
            chunk_size,
            &_hashes[(offset + o) / CHUNK_SIZE * hash_type::HASH_LEN]);

          remaining_size -= chunk_size;
        }
      }

      template <class encoder_type>
      inline std::string get_root_hash()
      {
        uint8_t root_hash[hash_type::HASH_LEN];

        hash::compute<hash_type>(_hashes, root_hash);

        return encoder::encode<encoder_type>(root_hash, hash_type::HASH_LEN);
      }

    private:
      std::vector<uint8_t> _hashes;
    };
  }
}

#endif
