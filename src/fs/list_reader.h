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

#include <list>
#include <string>

namespace s3 {
namespace base {
class Request;
};

namespace fs {
class ListReader {
 public:
  ListReader(const std::string &prefix, bool group_common_prefixes = true,
             int max_keys = -1);

  int Read(base::Request *req, std::list<std::string> *keys,
           std::list<std::string> *prefixes);

 private:
  const std::string prefix_;
  const bool group_common_prefixes_;
  const int max_keys_;
  std::string marker_;
  bool truncated_ = true;
};
}  // namespace fs
}  // namespace s3

#endif
