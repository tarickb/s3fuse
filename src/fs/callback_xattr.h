/*
 * fs/callback_xattr.h
 * -------------------------------------------------------------------------
 * Represents an object extended attribute whose value is read from and 
 * written to a pair of callback functions.
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

#ifndef S3_FS_CALLBACK_XATTR_H
#define S3_FS_CALLBACK_XATTR_H

#include <string>
#include <boost/function.hpp>

#include "fs/xattr.h"

namespace s3
{
  namespace fs
  {
    class callback_xattr : public xattr
    {
    public:
      typedef boost::function2<int, std::string, std::string *> get_value_function;
      typedef boost::function2<int, std::string, std::string> set_value_function;

      inline static ptr create(
        const std::string &key, 
        const get_value_function &get_fn,
        const set_value_function &set_fn)
      {
        return xattr::ptr(new callback_xattr(key, get_fn, set_fn));
      }

      virtual int set_value(const char *value, size_t size);
      virtual int get_value(char *buffer, size_t max_size);

      virtual void to_header(std::string *header, std::string *value);

    private:
      inline callback_xattr(
        const std::string &key, 
        const get_value_function &get_fn, 
        const set_value_function &set_fn
      ) : xattr(key, XM_DEFAULT),
          _get_fn(get_fn),
          _set_fn(set_fn)
      {
      }

      get_value_function _get_fn;
      set_value_function _set_fn;
      std::string _last_get_result;
    };
  }
}

#endif
