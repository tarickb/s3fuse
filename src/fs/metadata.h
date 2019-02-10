/*
 * fs/metadata.h
 * -------------------------------------------------------------------------
 * Shared filesystem metadata keys.
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

#ifndef S3_FS_METADATA_H
#define S3_FS_METADATA_H

namespace s3 {
namespace fs {
class Metadata {
 public:
  static constexpr char RESERVED_PREFIX[] = "s3fuse-";

  // this can't share a common prefix with RESERVED_PREFIX
  static constexpr char XATTR_PREFIX[] = "s3fuse_xattr_";

  static constexpr char LAST_UPDATE_ETAG[] = "s3fuse-lu-etag";
  static constexpr char MODE[] = "s3fuse-mode";
  static constexpr char UID[] = "s3fuse-uid";
  static constexpr char GID[] = "s3fuse-gid";
  static constexpr char CREATED_TIME[] = "s3fuse-ctime";
  static constexpr char LAST_MODIFIED_TIME[] = "s3fuse-mtime";

  static constexpr char FILE_TYPE[] = "s3fuse-file-type";
  static constexpr char DEVICE[] = "s3fuse-device";

  static constexpr char SHA256[] = "s3fuse-sha256";
  static constexpr char ENC_IV[] = "s3fuse-e-iv";
  static constexpr char ENC_METADATA[] = "s3fuse-e-meta";
};
}  // namespace fs
}  // namespace s3

#endif
