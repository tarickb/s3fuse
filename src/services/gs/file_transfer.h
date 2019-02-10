/*
 * services/gs/file_transfer.h
 * -------------------------------------------------------------------------
 * Google Storage file transfer class declaration.
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

#ifndef S3_SERVICES_GS_FILE_TRANSFER_H
#define S3_SERVICES_GS_FILE_TRANSFER_H

#include <string>

#include "services/file_transfer.h"

namespace s3 {
namespace services {
namespace gs {
class FileTransfer : public services::FileTransfer {
 public:
  FileTransfer();

  size_t upload_chunk_size() override;

 protected:
  int UploadMulti(const std::string &url, size_t size, const ReadChunk &on_read,
                  std::string *returned_etag) override;

 private:
  struct UploadRange {
    size_t size;
    off_t offset;
  };

  int ReadAndUpload(base::Request *req, const std::string &url,
                    const ReadChunk &on_read, UploadRange *range,
                    size_t total_size);

  int UploadPart(base::Request *req, const std::string &url,
                 const ReadChunk &on_read, UploadRange *range, bool is_retry);

  int UploadLastPart(base::Request *req, const std::string &url,
                     const ReadChunk &on_read, UploadRange *range,
                     size_t total_size, std::string *returned_etag);

  int UploadMultiInit(base::Request *req, const std::string &url,
                      std::string *location);

  size_t upload_chunk_size_;
};
}  // namespace gs
}  // namespace services
}  // namespace s3

#endif
