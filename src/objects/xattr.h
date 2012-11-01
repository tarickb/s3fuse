/*
 * xattr.h
 * -------------------------------------------------------------------------
 * Represents an object extended attribute.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#ifndef S3_OBJECTS_XATTR_H
#define S3_OBJECTS_XATTR_H

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace objects
  {
    class xattr
    {
    public:
      typedef boost::shared_ptr<xattr> ptr;

      enum access_mode
      {
        XM_DEFAULT = 0x0,
        XM_WRITABLE = 0x1,
        XM_SERIALIZABLE = 0x2
      };

      static ptr from_header(
        const std::string &header_key, 
        const std::string &header_value, 
        int mode = XM_DEFAULT);

      static ptr from_string(
        const std::string &key, 
        const std::string &value, 
        int mode = XM_DEFAULT);

      static ptr create(
        const std::string &key, 
        int mode = XM_DEFAULT);

      inline const std::string & get_key() const { return _key; }

      inline bool is_serializable() const { return _mode & XM_SERIALIZABLE; }
      inline bool is_writable() const { return _mode & XM_WRITABLE; }

      void set_value(const char *value, size_t size);
      int get_value(char *buffer, size_t max_size);

      void to_header(std::string *header, std::string *value);

    protected:
      inline xattr(const std::string &key, bool encode_key, bool encode_value, int mode)
        : _key(key),
          _encode_key(encode_key),
          _encode_value(encode_value),
          _mode(mode)
      {
      }

      std::string _key;
      std::vector<uint8_t> _value;
      bool _encode_key, _encode_value;
      int _mode;
    };

    class xattr_map : public std::map<std::string, xattr::ptr>
    {
    public:
      inline std::pair<iterator, bool> insert(const xattr::ptr &xa)
      {
        return std::map<std::string, xattr::ptr>::insert(std::make_pair(xa->get_key(), xa));
      }

      inline void replace(const xattr::ptr &xa)
      {
        (*this)[xa->get_key()] = xa;
      }
    };
  }
}

#endif
