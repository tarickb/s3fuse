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

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace objects
  {
    class xattr
    {
    public:
      typedef boost::shared_ptr<xattr> ptr;

      inline const std::string & get_key() const { return _key; }

      virtual bool is_serializable() = 0;
      virtual bool is_read_only() = 0;

      virtual void set_value(const char *value, size_t size) = 0;
      virtual int get_value(char *buffer, size_t max_size) = 0;

      virtual void to_header(std::string *header, std::string *value) = 0;

    protected:
      inline xattr(const std::string &key)
        : _key(key)
      {
      }

      std::string _key;
    };

    class xattr_map : public std::map<std::string, xattr::ptr>
    {
    public:
      inline std::pair<iterator, bool> insert(const xattr::ptr &xa)
      {
        return std::map<std::string, xattr::ptr>::insert(std::make_pair(xa->get_key(), xa));
      }
    };
  }
}

#endif
