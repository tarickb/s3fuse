/*
 * fs/static_xattr.h
 * -------------------------------------------------------------------------
 * Represents a static object extended attribute.
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

#ifndef S3_FS_STATIC_XATTR_H
#define S3_FS_STATIC_XATTR_H

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "fs/xattr.h"

namespace s3
{
  namespace fs
  {
    class static_xattr : public xattr
    {
    public:
      static ptr from_header(
        const std::string &header_key, 
        const std::string &header_value, 
        int mode);

      static ptr from_string(
        const std::string &key, 
        const std::string &value, 
        int mode);

      static ptr create(
        const std::string &key, 
        int mode);

      virtual int set_value(const char *value, size_t size);
      virtual int get_value(char *buffer, size_t max_size);

      virtual void to_header(std::string *header, std::string *value);

    private:
      inline static_xattr(const std::string &key, bool encode_key, bool encode_value, int mode)
        : xattr(key, mode),
          _encode_key(encode_key),
          _encode_value(encode_value)
      {
      }

      std::vector<uint8_t> _value;
      bool _encode_key, _encode_value;
    };
  }
}

#endif
