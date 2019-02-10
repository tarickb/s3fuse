/*
 * services/aws/file_transfer.h
 * -------------------------------------------------------------------------
 * AWS file transfer class declaration.
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

#ifndef S3_SERVICES_AWS_FILE_TRANSFER_H
#define S3_SERVICES_AWS_FILE_TRANSFER_H

#include <string>

#include "services/file_transfer.h"

namespace s3 {
namespace services {
namespace aws {
class FileTransfer : public services::FileTransfer {
 public:
  FileTransfer();

  size_t upload_chunk_size() override;

 protected:
  int UploadMulti(const std::string &url, size_t size, const ReadChunk &on_read,
                  std::string *returned_etag) override;

 private:
  struct UploadRange {
    int id;
    size_t size;
    off_t offset;
    std::string etag;
  };

  int UploadPart(base::Request *req, const std::string &url,
                 const std::string &upload_id, const ReadChunk &on_read,
                 UploadRange *range, bool is_retry);

  int UploadMultiInit(base::Request *req, const std::string &url,
                      std::string *upload_id);

  int UploadMultiCancel(base::Request *req, const std::string &url,
                        const std::string &upload_id);

  int UploadMultiComplete(base::Request *req, const std::string &url,
                          const std::string &upload_id,
                          const std::string &upload_metadata,
                          std::string *etag);

  size_t upload_chunk_size_;
};
}  // namespace aws
}  // namespace services
}  // namespace s3

#endif
