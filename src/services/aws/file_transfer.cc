/*
 * services/aws/file_transfer.cc
 * -------------------------------------------------------------------------
 * AWS file transfer implementation.
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

#include "services/aws/file_transfer.h"

#include <atomic>
#include <string>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "crypto/hash.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace services {
namespace aws {

namespace {
constexpr size_t UPLOAD_CHUNK_SIZE = 5 * 1024 * 1024;

constexpr char MULTIPART_ETAG_XPATH[] = "/CompleteMultipartUploadResult/ETag";
constexpr char MULTIPART_UPLOAD_ID_XPATH[] =
    "/InitiateMultipartUploadResult/UploadId";

std::atomic_int s_uploads_multi_chunks_failed(0);

void StatsWriter(std::ostream *o) {
  *o << "aws multi-part uploads:\n"
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
  std::string upload_id;
  int r;
  r = threads::Pool::Call(threads::PoolId::PR_REQ_0,
                          bind(&FileTransfer::UploadMultiInit, this,
                               std::placeholders::_1, url, &upload_id));
  if (r) return r;

  const size_t num_parts = (size + upload_chunk_size_ - 1) / upload_chunk_size_;
  std::vector<UploadRange> parts(num_parts);
  for (size_t i = 0; i < num_parts; i++) {
    UploadRange *part = &parts[i];

    part->id = i;
    part->offset = i * upload_chunk_size_;
    part->size = (i != num_parts - 1) ? upload_chunk_size_
                                      : (size - upload_chunk_size_ * i);
  }

  threads::ParallelWorkQueue<UploadRange> upload(
      parts.begin(), parts.end(),
      bind(&FileTransfer::UploadPart, this, std::placeholders::_1, url,
           upload_id, on_read, std::placeholders::_2, false),
      bind(&FileTransfer::UploadPart, this, std::placeholders::_1, url,
           upload_id, on_read, std::placeholders::_2, true));

  r = upload.Process();

  if (r) {
    threads::Pool::Call(threads::PoolId::PR_REQ_0,
                        bind(&FileTransfer::UploadMultiCancel, this,
                             std::placeholders::_1, url, upload_id));

    return r;
  }

  std::string complete_upload = "<CompleteMultipartUpload>";

  for (size_t i = 0; i < parts.size(); i++) {
    // part numbers are 1-based
    complete_upload += "<Part><PartNumber>" + std::to_string(i + 1) +
                       "</PartNumber><ETag>" + parts[i].etag + "</ETag></Part>";
  }

  complete_upload += "</CompleteMultipartUpload>";

  return threads::Pool::Call(
      threads::PoolId::PR_REQ_0,
      bind(&FileTransfer::UploadMultiComplete, this, std::placeholders::_1, url,
           upload_id, complete_upload, returned_etag));
}

int FileTransfer::UploadPart(base::Request *req, const std::string &url,
                             const std::string &upload_id,
                             const ReadChunk &on_read, UploadRange *range,
                             bool is_retry) {
  if (is_retry) ++s_uploads_multi_chunks_failed;

  std::vector<char> buffer;
  int r = on_read(range->size, range->offset, &buffer);
  if (r) return r;

  range->etag =
      crypto::Hash::Compute<crypto::Md5, crypto::HexWithQuotes>(buffer);

  req->Init(base::HttpMethod::PUT);

  // part numbers are 1-based
  req->SetUrl(url + "?partNumber=" + std::to_string(range->id + 1) +
              "&uploadId=" + upload_id);
  req->SetInputBuffer(std::move(buffer));

  req->Run(base::Config::transfer_timeout_in_s());

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  if (req->response_header("ETag") != range->etag) {
    S3_LOG(LOG_WARNING, "FileTransfer::upload_part",
           "md5 mismatch. expected %s, got %s.\n", range->etag.c_str(),
           req->response_header("ETag").c_str());
    return -EAGAIN;  // assume it's a temporary failure
  }

  return 0;
}

int FileTransfer::UploadMultiInit(base::Request *req, const std::string &url,
                                  std::string *upload_id) {
  req->Init(base::HttpMethod::POST);
  req->SetUrl(url + "?uploads");
  req->SetHeader("Content-Type", "");

  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "FileTransfer::upload_multi_init",
           "failed to parse response.\n");
    return -EIO;
  }

  int r = doc->Find(MULTIPART_UPLOAD_ID_XPATH, upload_id);
  if (r) return r;

  return upload_id->empty() ? -EIO : 0;
}

int FileTransfer::UploadMultiCancel(base::Request *req, const std::string &url,
                                    const std::string &upload_id) {
  S3_LOG(LOG_WARNING, "FileTransfer::upload_multi_cancel",
         "one or more parts failed to upload for [%s].\n", url.c_str());

  req->Init(base::HttpMethod::DELETE);
  req->SetUrl(url + "?uploadId=" + upload_id);

  req->Run();

  return 0;
}

int FileTransfer::UploadMultiComplete(base::Request *req,
                                      const std::string &url,
                                      const std::string &upload_id,
                                      const std::string &upload_metadata,
                                      std::string *etag) {
  req->Init(base::HttpMethod::POST);
  req->SetUrl(url + "?uploadId=" + upload_id);
  req->SetInputBuffer(upload_metadata);
  req->SetHeader("Content-Type", "");

  // use the transfer timeout because completing a multi-part upload can take a
  // long time see
  // http://docs.amazonwebservices.com/AmazonS3/latest/API/index.html?mpUploadComplete.html
  req->Run(base::Config::transfer_timeout_in_s());

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "FileTransfer::upload_multi_complete",
           "failed to complete multipart upload for [%s] with error %li.\n",
           url.c_str(), req->response_code());
    return -EIO;
  }

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "FileTransfer::upload_multi_complete",
           "failed to parse response.\n");
    return -EIO;
  }

  int r = doc->Find(MULTIPART_ETAG_XPATH, etag);
  if (r) return r;

  if (etag->empty()) {
    S3_LOG(LOG_WARNING, "FileTransfer::upload_multi_complete",
           "no etag on multipart upload of [%s]. response: %s\n", url.c_str(),
           req->GetOutputAsString().c_str());
    return -EIO;
  }

  return 0;
}

}  // namespace aws
}  // namespace services
}  // namespace s3
