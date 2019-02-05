/*
 * fs/list_reader.h
 * -------------------------------------------------------------------------
 * Bucket lister declaration.
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

#ifndef S3_FS_LIST_READER_H
#define S3_FS_LIST_READER_H

#include <memory>
#include <string>

#include "base/xml.h"

namespace s3 {
namespace base {
class request;
};

namespace fs {
class list_reader {
public:
  typedef std::shared_ptr<list_reader> ptr;

  list_reader(const std::string &prefix, bool group_common_prefixes = true,
              int max_keys = -1);

  int read(const std::shared_ptr<base::request> &req,
           base::xml::element_list *keys, base::xml::element_list *prefixes);

private:
  bool _truncated;
  std::string _prefix, _marker;
  bool _group_common_prefixes;
  int _max_keys;
};
} // namespace fs
} // namespace s3

#endif
