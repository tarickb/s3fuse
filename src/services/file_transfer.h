/*
 * services/file_transfer.h
 * -------------------------------------------------------------------------
 * File transfer base class.
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

#ifndef S3_SERVICES_FILE_TRANSFER_H
#define S3_SERVICES_FILE_TRANSFER_H

#include <functional>
#include <memory>
#include <string>

#include "base/request.h"

namespace s3 {
namespace services {
class FileTransfer {
 public:
  using WriteChunk = std::function<int(const char *, size_t, off_t)>;
  using ReadChunk = std::function<int(size_t, off_t, std::vector<char> *)>;

  virtual ~FileTransfer() = default;

  virtual size_t download_chunk_size();
  virtual size_t upload_chunk_size();

  int Download(const std::string &url, size_t size, const WriteChunk &on_write);
  int Upload(const std::string &url, size_t size, const ReadChunk &on_read,
             std::string *returned_etag);

 protected:
  virtual int DownloadSingle(base::Request *req, const std::string &url,
                             size_t size, const WriteChunk &on_write);

  virtual int DownloadMulti(const std::string &url, size_t size,
                            const WriteChunk &on_write);

  virtual int UploadSingle(base::Request *req, const std::string &url,
                           size_t size, const ReadChunk &on_read,
                           std::string *returned_etag);

  virtual int UploadMulti(const std::string &url, size_t size,
                          const ReadChunk &on_read, std::string *returned_etag);
};
}  // namespace services
}  // namespace s3

#endif
