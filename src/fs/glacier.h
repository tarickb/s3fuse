/*
 * fs/glacier.h
 * -------------------------------------------------------------------------
 * Glacier auto-archive support class.
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

#ifndef S3_FS_GLACIER_H
#define S3_FS_GLACIER_H

#include <list>
#include <memory>
#include <string>

namespace s3 {
namespace base {
class Request;
}
namespace fs {
class XAttr;

class Glacier {
 public:
  inline static std::unique_ptr<Glacier> Create(const std::string &path,
                                                const std::string &url,
                                                base::Request *req) {
    std::unique_ptr<Glacier> g(new Glacier(path, url));
    g->ExtractRestoreStatus(req);
    return g;
  }

  std::list<std::unique_ptr<XAttr>> BuildXAttrs();

 private:
  inline Glacier(const std::string &path, const std::string &url)
      : path_(path), url_(url) {}

  void ExtractRestoreStatus(base::Request *req);
  int QueryStorageClass(base::Request *req);
  int StartRestore(base::Request *req, int days);

  const std::string path_;
  const std::string url_;

  std::string storage_class_, restore_ongoing_, restore_expiry_;
};
}  // namespace fs
}  // namespace s3

#endif
