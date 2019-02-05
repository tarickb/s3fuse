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

#include <atomic>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "services/gs/file_transfer.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace services {
namespace gs {

namespace {
const size_t UPLOAD_CHUNK_SIZE = 256 * 1024;

const std::string UPLOAD_ID_DELIM = "?upload_id=";

std::atomic_int s_uploads_multi_chunks_failed(0);

void statistics_writer(std::ostream *o) {
  *o << "google storage multi-part uploads:\n"
        "  chunks failed: "
     << s_uploads_multi_chunks_failed << "\n";
}

base::statistics::writers::entry s_writer(statistics_writer, 0);
} // namespace

file_transfer::file_transfer() {
  _upload_chunk_size = (base::config::get_upload_chunk_size() == -1)
                           ? UPLOAD_CHUNK_SIZE
                           : base::config::get_upload_chunk_size();
}

size_t file_transfer::get_upload_chunk_size() { return _upload_chunk_size; }

int file_transfer::upload_multi(const std::string &url, size_t size,
                                const read_chunk_fn &on_read,
                                std::string *returned_etag) {
  typedef threads::parallel_work_queue<upload_range> multipart_upload;

  std::string location;
  const size_t num_parts = (size + _upload_chunk_size - 1) / _upload_chunk_size;
  std::vector<upload_range> parts(num_parts);
  upload_range last_part;
  std::unique_ptr<multipart_upload> upload;
  int r;

  r = threads::pool::call(threads::PR_REQ_0,
                          bind(&file_transfer::upload_multi_init, this,
                               std::placeholders::_1, url, &location));

  if (r)
    return r;

  for (size_t i = 0; i < num_parts; i++) {
    upload_range *part = &parts[i];

    part->offset = i * _upload_chunk_size;
    part->size = (i != num_parts - 1) ? _upload_chunk_size
                                      : (size - _upload_chunk_size * i);
  }

  last_part = parts.back();
  parts.pop_back();

  upload.reset(new multipart_upload(
      parts.begin(), parts.end(),
      bind(&file_transfer::upload_part, this, std::placeholders::_1, location,
           on_read, std::placeholders::_2, false),
      bind(&file_transfer::upload_part, this, std::placeholders::_1, location,
           on_read, std::placeholders::_2, true),
      -1,  // default max_retries
      1)); // only one part at a time

  r = upload->process();

  if (r)
    return r;

  return threads::pool::call(threads::PR_REQ_0,
                             bind(&file_transfer::upload_last_part, this,
                                  std::placeholders::_1, location, on_read,
                                  &last_part, size, returned_etag));
}

int file_transfer::read_and_upload(const base::request::ptr &req,
                                   const std::string &url,
                                   const read_chunk_fn &on_read,
                                   upload_range *range, size_t total_size) {
  int r = 0;
  base::char_vector_ptr buffer(new base::char_vector());
  std::string content_range = "bytes ";

  r = on_read(range->size, range->offset, buffer);

  if (r)
    return r;

  req->init(base::HTTP_PUT);

  req->set_url(url);
  req->set_input_buffer(buffer);

  content_range +=
      std::to_string(range->offset) + "-" +
      std::to_string(range->offset + range->size - 1) + "/" +
      ((total_size == 0) ? std::string("*") : std::to_string(total_size));

  req->set_header("Content-Range", content_range);

  req->run(base::config::get_transfer_timeout_in_s());

  return 0;
}

int file_transfer::upload_part(const base::request::ptr &req,
                               const std::string &url,
                               const read_chunk_fn &on_read,
                               upload_range *range, bool is_retry) {
  int r = 0;

  if (is_retry)
    ++s_uploads_multi_chunks_failed;

  r = read_and_upload(req, url, on_read, range, 0);

  if (r)
    return r;

  return (req->get_response_code() != base::HTTP_SC_RESUME) ? -EIO : 0;
}

int file_transfer::upload_last_part(const base::request::ptr &req,
                                    const std::string &url,
                                    const read_chunk_fn &on_read,
                                    upload_range *range, size_t total_size,
                                    std::string *returned_etag) {
  int r = 0;

  r = read_and_upload(req, url, on_read, range, total_size);

  if (r)
    return r;

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  *returned_etag = req->get_response_header("ETag");

  return 0;
}

int file_transfer::upload_multi_init(const base::request::ptr &req,
                                     const std::string &url,
                                     std::string *location) {
  size_t pos = 0;

  req->init(base::HTTP_POST);
  req->set_url(url);
  req->set_header("x-goog-resumable", "start");

  req->run();

  if (req->get_response_code() != base::HTTP_SC_CREATED)
    return -EIO;

  *location = req->get_response_header("Location");

  pos = location->find(UPLOAD_ID_DELIM);

  if (pos == std::string::npos)
    return -EIO;

  *location = url + location->substr(pos);

  return 0;
}

} // namespace gs
} // namespace services
} // namespace s3
