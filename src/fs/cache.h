/*
 * fs/cache.h
 * -------------------------------------------------------------------------
 * Caches object (i.e., file, directory, symlink) metadata.
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

#ifndef S3_FS_CACHE_H
#define S3_FS_CACHE_H

#include <functional>
#include <memory>
#include <string>

namespace s3 {
namespace base {
class Request;
}

namespace fs {
enum class CacheHints { NONE, IS_DIR, IS_FILE };

class Object;

class Cache {
 public:
  using LockedObjectCallback = std::function<void(std::shared_ptr<Object>)>;

  static void Init();

  static std::shared_ptr<Object> Get(const std::string &path,
                                     CacheHints hints = CacheHints::NONE);

  static int Preload(base::Request *req, const std::string &path,
                     CacheHints hints = CacheHints::NONE);

  static int Remove(const std::string &path);

  // this method is intended to ensure that callback() is called on the one and
  // only cached object at "path"
  static void LockObject(const std::string &path,
                         const LockedObjectCallback &callback);
};
}  // namespace fs
}  // namespace s3

#endif
