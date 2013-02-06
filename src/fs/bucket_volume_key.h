/*
 * fs/bucket_volume_key.h
 * -------------------------------------------------------------------------
 * Manages in-bucket volume keys.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#ifndef S3_FS_BUCKET_VOLUME_KEY_H
#define S3_FS_BUCKET_VOLUME_KEY_H

#include <boost/smart_ptr.hpp>

#include "base/request.h"
#include "crypto/aes_cbc_256.h"

namespace s3
{
  namespace crypto
  {
    class buffer;
  }

  namespace fs
  {
    class bucket_volume_key
    {
    public:
      typedef crypto::aes_cbc_256_with_pkcs key_cipher;

      typedef boost::shared_ptr<bucket_volume_key> ptr;

      static ptr fetch(const base::request::ptr &req, const std::string &id);
      static ptr generate(const base::request::ptr &req, const std::string &id);
      static void get_keys(const base::request::ptr &req, std::vector<std::string> *keys);

      inline const boost::shared_ptr<crypto::buffer> & get_volume_key() { return _volume_key; }
      inline const std::string & get_id() { return _id; }

      void unlock(const boost::shared_ptr<crypto::buffer> &key);
      void remove(const base::request::ptr &req);
      void commit(const base::request::ptr &req, const boost::shared_ptr<crypto::buffer> &key);

      ptr clone(const std::string &new_id);

    private:
      bucket_volume_key(const std::string &id);

      inline bool is_present() { return !_encrypted_key.empty(); }

      void download(const base::request::ptr &req);
      void generate();

      std::string _id, _encrypted_key;
      boost::shared_ptr<crypto::buffer> _volume_key;
    };
  }
}

#endif
