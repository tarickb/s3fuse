/*
 * fs/xattr.h
 * -------------------------------------------------------------------------
 * Base class for object extended attributes.
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

#ifndef S3_FS_XATTR_H
#define S3_FS_XATTR_H

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace fs
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

      inline const std::string & get_key() const { return _key; }

      inline bool is_serializable() const { return _mode & XM_SERIALIZABLE; }
      inline bool is_writable() const { return _mode & XM_WRITABLE; }

      virtual int set_value(const char *value, size_t size) = 0;
      virtual int get_value(char *buffer, size_t max_size) = 0;

      virtual void to_header(std::string *header, std::string *value) = 0;

    protected:
      inline xattr(const std::string &key, int mode)
        : _key(key),
          _mode(mode)
      {
      }

    private:
      std::string _key;
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
