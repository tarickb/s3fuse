/*
 * services/file_transfer.cc
 * -------------------------------------------------------------------------
 * File transfer base class implementation.
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

#include "services/file_transfer.h"

#include <atomic>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace services {

namespace {
struct DownloadRange {
  size_t size;
  off_t offset;
};

std::atomic_int s_downloads_single(0), s_downloads_single_failed(0);
std::atomic_int s_downloads_multi(0), s_downloads_multi_failed(0),
    s_downloads_multi_chunks_failed(0);
std::atomic_int s_uploads_single(0), s_uploads_single_failed(0);
std::atomic_int s_uploads_multi(0), s_uploads_multi_failed(0);

void StatsWriter(std::ostream *o) {
  *o << "common single-part downloads:\n"
        "  succeeded: "
     << s_downloads_single
     << "\n"
        "  failed: "
     << s_downloads_single_failed
     << "\n"
        "common multi-part downloads:\n"
        "  succeeded: "
     << s_downloads_multi
     << "\n"
        "  failed: "
     << s_downloads_multi_failed
     << "\n"
        "  chunks failed: "
     << s_downloads_multi_chunks_failed
     << "\n"
        "common single-part uploads:\n"
        "  succeeded: "
     << s_uploads_single
     << "\n"
        "  failed: "
     << s_uploads_single_failed
     << "\n"
        "common multi-part uploads:\n"
        "  succeeded: "
     << s_uploads_multi
     << "\n"
        "  failed: "
     << s_uploads_multi_failed << "\n";
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);

int DownloadPart(base::Request *req, const std::string &url,
                 DownloadRange *range, const FileTransfer::WriteChunk &on_write,
                 bool is_retry) {
  // yes, relying on is_retry will result in the chunks failed count being off
  // by one, maybe, but we don't care
  if (is_retry) ++s_downloads_multi_chunks_failed;

  req->Init(base::HttpMethod::GET);
  req->SetUrl(url);
  req->SetHeader("Range", std::string("bytes=") +
                              std::to_string(range->offset) + std::string("-") +
                              std::to_string(range->offset + range->size));

  req->Run(base::Config::transfer_timeout_in_s());

  if (req->response_code() != s3::base::HTTP_SC_PARTIAL_CONTENT)
    return -EIO;
  else if (req->output_buffer().size() < range->size)
    return -EIO;

  return on_write(&req->output_buffer()[0], range->size, range->offset);
}

int IncrementOnResult(int r, std::atomic_int *success,
                      std::atomic_int *failure) {
  if (r)
    ++(*failure);
  else
    ++(*success);

  return r;
}
}  // namespace

size_t FileTransfer::download_chunk_size() {
  return base::Config::download_chunk_size();
}

size_t FileTransfer::upload_chunk_size() {
  return 0;  // this FileTransfer impl doesn't do chunks
}

int FileTransfer::Download(const std::string &url, size_t size,
                           const WriteChunk &on_write) {
  if (download_chunk_size() > 0 && size > download_chunk_size())
    return IncrementOnResult(DownloadMulti(url, size, on_write),
                             &s_downloads_multi, &s_downloads_multi_failed);
  else
    return IncrementOnResult(
        threads::Pool::Call(threads::PoolId::PR_REQ_1,
                            bind(&FileTransfer::DownloadSingle, this,
                                 std::placeholders::_1, url, size, on_write)),
        &s_downloads_single, &s_downloads_single_failed);
}

int FileTransfer::Upload(const std::string &url, size_t size,
                         const ReadChunk &on_read, std::string *returned_etag) {
  if (upload_chunk_size() > 0 && size > upload_chunk_size())
    return IncrementOnResult(UploadMulti(url, size, on_read, returned_etag),
                             &s_uploads_multi, &s_uploads_multi_failed);
  else
    return IncrementOnResult(
        threads::Pool::Call(
            threads::PoolId::PR_REQ_1,
            bind(&FileTransfer::UploadSingle, this, std::placeholders::_1, url,
                 size, on_read, returned_etag)),
        &s_uploads_single, &s_uploads_single_failed);
}

int FileTransfer::DownloadSingle(base::Request *req, const std::string &url,
                                 size_t size, const WriteChunk &on_write) {
  long rc = 0;

  req->Init(base::HttpMethod::GET);
  req->SetUrl(url);

  req->Run(base::Config::transfer_timeout_in_s());
  rc = req->response_code();

  if (rc == base::HTTP_SC_NOT_FOUND)
    return -ENOENT;
  else if (rc != base::HTTP_SC_OK)
    return -EIO;

  return on_write(&req->output_buffer()[0], req->output_buffer().size(), 0);
}

int FileTransfer::DownloadMulti(const std::string &url, size_t size,
                                const FileTransfer::WriteChunk &on_write) {
  size_t num_parts = (size + download_chunk_size() - 1) / download_chunk_size();
  std::vector<DownloadRange> parts(num_parts);

  for (size_t i = 0; i < num_parts; i++) {
    DownloadRange *range = &parts[i];

    range->offset = i * download_chunk_size();
    range->size = (i != num_parts - 1) ? download_chunk_size()
                                       : (size - download_chunk_size() * i);
  }

  threads::ParallelWorkQueue<DownloadRange> dl(
      parts.begin(), parts.end(),
      bind(&DownloadPart, std::placeholders::_1, url, std::placeholders::_2,
           on_write, false),
      bind(&DownloadPart, std::placeholders::_1, url, std::placeholders::_2,
           on_write, true));
  return dl.Process();
}

int FileTransfer::UploadSingle(base::Request *req, const std::string &url,
                               size_t size, const ReadChunk &on_read,
                               std::string *returned_etag) {
  std::vector<char> buffer;
  int r = on_read(size, 0, &buffer);
  if (r) return r;

  uint8_t read_hash[crypto::Md5::HASH_LEN];
  crypto::Hash::Compute<crypto::Md5>(buffer, read_hash);

  const std::string expected_md5_b64 =
      crypto::Encoder::Encode<crypto::Base64>(read_hash, crypto::Md5::HASH_LEN);
  const std::string expected_md5_hex =
      crypto::Encoder::Encode<crypto::HexWithQuotes>(read_hash,
                                                     crypto::Md5::HASH_LEN);

  req->Init(base::HttpMethod::PUT);
  req->SetUrl(url);

  req->SetHeader("Content-MD5", expected_md5_b64);
  req->SetInputBuffer(std::move(buffer));

  req->Run(base::Config::transfer_timeout_in_s());

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "FileTransfer::UploadSingle",
           "failed to upload for [%s].\n", url.c_str());
    return -EIO;
  }

  const std::string etag = req->response_header("ETag");

  if (crypto::Md5::IsValidQuotedHexHash(etag) && etag != expected_md5_hex) {
    S3_LOG(LOG_WARNING, "FileTransfer::UploadSingle",
           "etag [%s] does not match md5 [%s].\n", etag.c_str(),
           expected_md5_hex.c_str());
    return -EIO;
  }

  *returned_etag = etag;

  return 0;
}

int FileTransfer::UploadMulti(const std::string &url, size_t size,
                              const ReadChunk &on_read,
                              std::string *returned_etag) {
  return -ENOTSUP;
}

}  // namespace services
}  // namespace s3
