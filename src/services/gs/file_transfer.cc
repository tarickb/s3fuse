/*
 * services/gs/file_transfer.cc
 * -------------------------------------------------------------------------
 * Google Storage file transfer implementation.
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

#include "services/gs/file_transfer.h"

#include <atomic>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace services {
namespace gs {

namespace {
const size_t UPLOAD_CHUNK_SIZE = 256 * 1024;

const std::string UPLOAD_ID_DELIM = "?upload_id=";

std::atomic_int s_uploads_multi_chunks_failed(0);

void StatsWriter(std::ostream *o) {
  *o << "google storage multi-part uploads:\n"
        "  chunks failed: "
     << s_uploads_multi_chunks_failed << "\n";
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

FileTransfer::FileTransfer() {
  upload_chunk_size_ = (base::Config::upload_chunk_size() == -1)
                           ? UPLOAD_CHUNK_SIZE
                           : base::Config::upload_chunk_size();
}

size_t FileTransfer::upload_chunk_size() { return upload_chunk_size_; }

int FileTransfer::UploadMulti(const std::string &url, size_t size,
                              const ReadChunk &on_read,
                              std::string *returned_etag) {
  std::string location;
  int r = threads::Pool::Call(threads::PoolId::PR_REQ_0,
                              bind(&FileTransfer::UploadMultiInit, this,
                                   std::placeholders::_1, url, &location));
  if (r) return r;

  const size_t num_parts = (size + upload_chunk_size_ - 1) / upload_chunk_size_;
  std::vector<UploadRange> parts(num_parts);
  for (size_t i = 0; i < num_parts; i++) {
    UploadRange *part = &parts[i];
    part->offset = i * upload_chunk_size_;
    part->size = (i != num_parts - 1) ? upload_chunk_size_
                                      : (size - upload_chunk_size_ * i);
  }

  UploadRange last_part = parts.back();
  parts.pop_back();

  threads::ParallelWorkQueue<UploadRange> upload(
      parts.begin(), parts.end(),
      bind(&FileTransfer::UploadPart, this, std::placeholders::_1, location,
           on_read, std::placeholders::_2, false),
      bind(&FileTransfer::UploadPart, this, std::placeholders::_1, location,
           on_read, std::placeholders::_2, true),
      -1,  // default max_retries
      1);  // only one part at a time

  r = upload.Process();
  if (r) return r;

  return threads::Pool::Call(
      threads::PoolId::PR_REQ_0,
      bind(&FileTransfer::UploadLastPart, this, std::placeholders::_1, location,
           on_read, &last_part, size, returned_etag));
}

int FileTransfer::ReadAndUpload(base::Request *req, const std::string &url,
                                const ReadChunk &on_read, UploadRange *range,
                                size_t total_size) {
  std::vector<char> buffer;
  int r = on_read(range->size, range->offset, &buffer);
  if (r) return r;

  req->Init(base::HttpMethod::PUT);
  req->SetUrl(url);
  req->SetInputBuffer(std::move(buffer));

  const std::string content_range =
      "bytes " + std::to_string(range->offset) + "-" +
      std::to_string(range->offset + range->size - 1) + "/" +
      ((total_size == 0) ? std::string("*") : std::to_string(total_size));
  req->SetHeader("Content-Range", content_range);

  req->Run(base::Config::transfer_timeout_in_s());
  return 0;
}

int FileTransfer::UploadPart(base::Request *req, const std::string &url,
                             const ReadChunk &on_read, UploadRange *range,
                             bool is_retry) {
  if (is_retry) ++s_uploads_multi_chunks_failed;

  int r = ReadAndUpload(req, url, on_read, range, 0);
  if (r) return r;

  return (req->response_code() != base::HTTP_SC_RESUME) ? -EIO : 0;
}

int FileTransfer::UploadLastPart(base::Request *req, const std::string &url,
                                 const ReadChunk &on_read, UploadRange *range,
                                 size_t total_size,
                                 std::string *returned_etag) {
  int r = ReadAndUpload(req, url, on_read, range, total_size);
  if (r) return r;

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  *returned_etag = req->response_header("ETag");
  return 0;
}

int FileTransfer::UploadMultiInit(base::Request *req, const std::string &url,
                                  std::string *location) {
  req->Init(base::HttpMethod::POST);
  req->SetUrl(url);
  req->SetHeader("x-goog-resumable", "start");

  req->Run();
  if (req->response_code() != base::HTTP_SC_CREATED) return -EIO;

  *location = req->response_header("Location");
  size_t pos = location->find(UPLOAD_ID_DELIM);
  if (pos == std::string::npos) return -EIO;

  *location = url + location->substr(pos);
  return 0;
}

}  // namespace gs
}  // namespace services
}  // namespace s3
