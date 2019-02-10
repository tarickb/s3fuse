/*
 * fs/bucket_volume_key.h
 * -------------------------------------------------------------------------
 * Manages in-bucket volume keys.
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

#ifndef S3_FS_BUCKET_VOLUME_KEY_H
#define S3_FS_BUCKET_VOLUME_KEY_H

#include <memory>

#include "base/request.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/buffer.h"

namespace s3 {
namespace fs {
class BucketVolumeKey {
 public:
  using KeyCipherType = crypto::AesCbc256WithPkcs;

  static std::unique_ptr<BucketVolumeKey> Fetch(base::Request *req,
                                                const std::string &id);
  static std::unique_ptr<BucketVolumeKey> Generate(base::Request *req,
                                                   const std::string &id);
  static std::vector<std::string> GetKeys(base::Request *req);

  inline const crypto::Buffer &volume_key() const { return volume_key_; }
  inline std::string id() const { return id_; }

  void Unlock(const crypto::Buffer &key);
  void Remove(base::Request *req);
  void Commit(base::Request *req, const crypto::Buffer &key);

  std::unique_ptr<BucketVolumeKey> Clone(const std::string &new_id) const;

 private:
  BucketVolumeKey(const std::string &id);

  inline bool is_present() const { return !encrypted_key_.empty(); }

  void Download(base::Request *req);
  void Generate();

  std::string id_, encrypted_key_;
  crypto::Buffer volume_key_;
};
}  // namespace fs
}  // namespace s3

#endif
