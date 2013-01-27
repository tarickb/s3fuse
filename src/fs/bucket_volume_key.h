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

      static bool is_present();

      static boost::shared_ptr<crypto::buffer> read(const boost::shared_ptr<crypto::buffer> &key);

      static void generate(const boost::shared_ptr<crypto::buffer> &key);
      static void reencrypt(const boost::shared_ptr<crypto::buffer> &old_key, const boost::shared_ptr<crypto::buffer> &new_key);
      static void remove();
    };
  }
}

#endif
