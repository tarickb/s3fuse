/*
 * fs/encrypted_file.h
 * -------------------------------------------------------------------------
 * Represents an encrypted file object.
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

#ifndef S3_FS_ENCRYPTED_FILE_H
#define S3_FS_ENCRYPTED_FILE_H

#include <string>
#include <vector>

#include "crypto/symmetric_key.h"
#include "fs/file.h"

namespace s3 {
namespace base {
class Request;
}

namespace fs {
class EncryptedFile : public File {
 public:
  EncryptedFile(const std::string &path);
  ~EncryptedFile() override = default;

 protected:
  void Init(base::Request *req) override;
  void SetRequestHeaders(base::Request *req) override;

  int IsDownloadable() override;

  int WriteChunk(const char *buffer, size_t size, off_t offset) override;
  int ReadChunk(size_t size, off_t offset, std::vector<char> *buffer) override;

  int PrepareUpload() override;
  int FinalizeUpload(const std::string &returned_etag) override;

 private:
  crypto::SymmetricKey meta_key_ = crypto::SymmetricKey::Empty();
  crypto::SymmetricKey data_key_ = crypto::SymmetricKey::Empty();
  std::string enc_iv_, enc_meta_;
};
}  // namespace fs
}  // namespace s3

#endif
