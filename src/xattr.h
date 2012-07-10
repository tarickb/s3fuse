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

#ifndef S3_XATTR_H
#define S3_XATTR_H

#include <stdint.h>

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace s3
{
  class xattr
  {
  public:
    typedef boost::shared_ptr<xattr> ptr;

    static ptr from_string(const std::string &key, const std::string &value);
    static ptr from_header(const std::string &header_key, const std::string &header_value);
    static ptr create(const std::string &key);

    void set_value(const char *value, size_t size);
    int get_value(char *buffer, size_t max_size);

    inline const std::string & get_key() { return _key; }

    void to_header(std::string *header, std::string *value);

  private:
    xattr(const std::string &key, bool key_needs_encoding);

    std::string _key;
    std::vector<uint8_t> _value;

    bool _key_needs_encoding, _value_needs_encoding;
    bool _is_serializable;
  };
}

#endif
