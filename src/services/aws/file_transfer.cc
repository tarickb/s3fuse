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
#include "services/aws/file_transfer.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
  namespace services {
    namespace aws {

namespace
{
  const size_t UPLOAD_CHUNK_SIZE = 5 * 1024 * 1024;

  const char *MULTIPART_ETAG_XPATH = "/CompleteMultipartUploadResult/ETag";
  const char *MULTIPART_UPLOAD_ID_XPATH = "/InitiateMultipartUploadResult/UploadId";

  std::atomic_int s_uploads_multi_chunks_failed(0);

  void statistics_writer(std::ostream *o)
  {
    *o <<
      "aws multi-part uploads:\n"
      "  chunks failed: " << s_uploads_multi_chunks_failed << "\n";
  }

  base::statistics::writers::entry s_writer(statistics_writer, 0);
}

file_transfer::file_transfer()
{
  _upload_chunk_size = 
    (base::config::get_upload_chunk_size() == -1)
      ? UPLOAD_CHUNK_SIZE
      : base::config::get_upload_chunk_size();
}

size_t file_transfer::get_upload_chunk_size()
{
  return _upload_chunk_size;
}

int file_transfer::upload_multi(const std::string &url, size_t size, const
    read_chunk_fn &on_read, std::string *returned_etag)
{
  typedef threads::parallel_work_queue<upload_range> multipart_upload;

  std::string upload_id, complete_upload;
  const size_t num_parts = (size + _upload_chunk_size - 1) / _upload_chunk_size;
  std::vector<upload_range> parts(num_parts);
  std::unique_ptr<multipart_upload> upload;
  int r;

  r = threads::pool::call(threads::PR_REQ_0,
      bind(&file_transfer::upload_multi_init, this, std::placeholders::_1, url, &upload_id));

  if (r)
    return r;

  for (size_t i = 0; i < num_parts; i++) {
    upload_range *part = &parts[i];

    part->id = i;
    part->offset = i * _upload_chunk_size;
    part->size = (i != num_parts - 1) ? _upload_chunk_size : (size - _upload_chunk_size * i);
  }

  upload.reset(new multipart_upload(
    parts.begin(),
    parts.end(),
    bind(&file_transfer::upload_part, this, std::placeholders::_1, url,
      upload_id, on_read, std::placeholders::_2, false),
    bind(&file_transfer::upload_part, this, std::placeholders::_1, url,
      upload_id, on_read, std::placeholders::_2, true)));

  r = upload->process();

  if (r) {
    threads::pool::call(
      threads::PR_REQ_0, 
      bind(&file_transfer::upload_multi_cancel, this, std::placeholders::_1, url, upload_id));

    return r;
  }

  complete_upload = "<CompleteMultipartUpload>";

  for (size_t i = 0; i < parts.size(); i++) {
    // part numbers are 1-based
    complete_upload += "<Part><PartNumber>" + std::to_string(i + 1) + "</PartNumber><ETag>" + parts[i].etag + "</ETag></Part>";
  }

  complete_upload += "</CompleteMultipartUpload>";

  return threads::pool::call(
    threads::PR_REQ_0, 
    bind(&file_transfer::upload_multi_complete, this, std::placeholders::_1, url, upload_id, complete_upload, returned_etag));
}

int file_transfer::upload_part(
  const base::request::ptr &req, 
  const std::string &url, 
  const std::string &upload_id, 
  const read_chunk_fn &on_read, 
  upload_range *range, 
  bool is_retry)
{
  int r = 0;
  base::char_vector_ptr buffer(new base::char_vector());

  if (is_retry)
    ++s_uploads_multi_chunks_failed;

  r = on_read(range->size, range->offset, buffer);

  if (r)
    return r;

  range->etag = crypto::hash::compute<crypto::md5, crypto::hex_with_quotes>(*buffer);

  req->init(base::HTTP_PUT);

  // part numbers are 1-based
  req->set_url(url + "?partNumber=" + std::to_string(range->id + 1) + "&uploadId=" + upload_id);
  req->set_input_buffer(buffer);

  req->run(base::config::get_transfer_timeout_in_s());

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  if (req->get_response_header("ETag") != range->etag) {
    S3_LOG(LOG_WARNING, "file_transfer::upload_part", "md5 mismatch. expected %s, got %s.\n", range->etag.c_str(), req->get_response_header("ETag").c_str());
    return -EAGAIN; // assume it's a temporary failure
  }

  return 0;
}

int file_transfer::upload_multi_init(const base::request::ptr &req, const std::string
    &url, std::string *upload_id)
{
  base::xml::document_ptr doc;
  int r;

  req->init(base::HTTP_POST);
  req->set_url(url + "?uploads");
  req->set_header("Content-Type", "");

  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  doc = base::xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file_transfer::upload_multi_init", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = base::xml::find(doc, MULTIPART_UPLOAD_ID_XPATH, upload_id)))
    return r;

  if (upload_id->empty())
    return -EIO;

  return r;
}

int file_transfer::upload_multi_cancel(const base::request::ptr &req, const
    std::string &url, const std::string &upload_id)
{
  S3_LOG(LOG_WARNING, "file_transfer::upload_multi_cancel", "one or more parts failed to upload for [%s].\n", url.c_str());

  req->init(base::HTTP_DELETE);
  req->set_url(url + "?uploadId=" + upload_id);

  req->run();

  return 0;
}

int file_transfer::upload_multi_complete(
  const base::request::ptr &req, 
  const std::string &url, 
  const std::string &upload_id, 
  const std::string &upload_metadata, 
  std::string *etag)
{
  base::xml::document_ptr doc;
  int r;

  req->init(base::HTTP_POST);
  req->set_url(url + "?uploadId=" + upload_id);
  req->set_input_buffer(upload_metadata);
  req->set_header("Content-Type", "");

  // use the transfer timeout because completing a multi-part upload can take a long time
  // see http://docs.amazonwebservices.com/AmazonS3/latest/API/index.html?mpUploadComplete.html
  req->run(base::config::get_transfer_timeout_in_s());

  if (req->get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "file_transfer::upload_multi_complete", "failed to complete multipart upload for [%s] with error %li.\n", url.c_str(), req->get_response_code());
    return -EIO;
  }

  doc = base::xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file_transfer::upload_multi_complete", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = base::xml::find(doc, MULTIPART_ETAG_XPATH, etag)))
    return r;

  if (etag->empty()) {
    S3_LOG(LOG_WARNING, "file_transfer::upload_multi_complete", "no etag on multipart upload of [%s]. response: %s\n", url.c_str(), req->get_output_string().c_str());
    return -EIO;
  }

  return 0;
}

} } }
