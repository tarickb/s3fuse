/*
 * fs/object.h
 * -------------------------------------------------------------------------
 * Base class for S3 objects.
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

#ifndef S3_FS_OBJECT_H
#define S3_FS_OBJECT_H

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/static_list.h"
#include "fs/xattr.h"
#include "services/versioning.h"
#include "threads/pool.h"

namespace s3 {
namespace base {
class Request;
}

namespace fs {
#ifdef WITH_AWS
class Glacier;
#endif

class Object {
 public:
  using TypeChecker =
      std::function<Object *(const std::string &path, base::Request *req)>;
  using TypeCheckers = base::StaticList<TypeChecker>;

  static int block_size();
  static std::string internal_prefix();

  static std::string BuildUrl(const std::string &path);
  static std::string BuildInternalUrl(const std::string &key);
  static bool IsInternalPath(const std::string &key);
  static bool IsVersionedPath(const std::string &path);

  static int RemoveByUrl(base::Request *req, const std::string &url);
  static int CopyByPath(base::Request *req, const std::string &from,
                        const std::string &to);

  static std::shared_ptr<Object> Create(const std::string &path,
                                        base::Request *req);

  virtual ~Object();

  virtual bool IsRemovable();
  virtual int Remove(base::Request *req);
  virtual int Rename(base::Request *req, std::string to);

  inline bool intact() const { return intact_; }
  inline bool expired() const {
    return (expiry_ == 0 || time(nullptr) >= expiry_);
  }

  inline std::string path() const { return path_; }
  inline std::string content_type() const { return content_type_; }
  inline std::string url() const { return url_; }
  inline std::string etag() const { return etag_; }
  inline mode_t mode() const { return stat_.st_mode; }
  inline mode_t type() const { return stat_.st_mode & S_IFMT; }
  inline uid_t uid() const { return stat_.st_uid; }

  inline void set_uid(uid_t uid) { stat_.st_uid = uid; }
  inline void set_gid(gid_t gid) { stat_.st_gid = gid; }

  inline time_t mtime() const { return stat_.st_mtime; }
  inline void set_mtime(time_t mtime) { stat_.st_mtime = mtime; }
  inline void set_mtime() { set_mtime(time(nullptr)); }

  inline void set_ctime(time_t ctime) { stat_.st_ctime = ctime; }
  inline void set_ctime() { set_ctime(time(nullptr)); }

  std::vector<std::string> GetMetadataKeys();
  int GetMetadata(const std::string &key, char *buffer, size_t max_size);
  int SetMetadata(const std::string &key, const char *value, size_t size,
                  int flags, bool *needs_commit);
  int RemoveMetadata(const std::string &key);

  void SetMode(mode_t mode);

  inline void CopyStat(struct stat *s) {
    UpdateStat();
    memcpy(s, &stat_, sizeof(stat_));
  }

  int Commit(base::Request *req);
  int Commit();

  int Remove();
  int Rename(std::string to);

 protected:
  using MetadataMap = std::map<std::string, std::unique_ptr<XAttr>>;

  explicit Object(const std::string &path);

  virtual void Init(base::Request *req);

  virtual void SetRequestHeaders(base::Request *req);
  virtual void SetRequestBody(base::Request *req);

  virtual void UpdateStat();

  inline struct stat *stat() { return &stat_; }

  inline void set_url(const std::string &url) { url_ = url; }
  inline void set_content_type(const std::string &content_type) {
    content_type_ = content_type;
  }
  inline void set_etag(const std::string &etag) { etag_ = etag; }
  inline void set_type(mode_t mode) {
    stat_.st_mode &= ~S_IFMT;  // clear existing mode
    stat_.st_mode |= mode & S_IFMT;
  }

  inline void Expire() { expiry_ = 0; }
  inline void ForceZeroSize() { stat_.st_size = 0; }

  MetadataMap::iterator UpdateMetadata(std::unique_ptr<XAttr> attr);

 private:
  int FetchAllVersions(services::VersionFetchOptions options,
                       base::Request *req, std::string *out);

  std::mutex mutex_;

  // should only be modified during init()
  std::string path_;
  std::string content_type_;
  std::string url_;
  bool intact_;

#ifdef WITH_AWS
  std::unique_ptr<Glacier> glacier_;
#endif

  // unprotected
  std::string etag_;
  struct stat stat_;
  time_t expiry_;

  // protected by _mutex
  MetadataMap metadata_;
};
}  // namespace fs
}  // namespace s3

#endif
