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

#include <boost/lexical_cast.hpp>
#include <boost/detail/atomic_count.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/statistics.h"
#include "services/gs/file_transfer.h"

using boost::lexical_cast;
using boost::detail::atomic_count;
using std::ostream;
using std::string;

using s3::base::config;
using s3::base::statistics;
using s3::services::gs::file_transfer;

namespace
{
  const size_t UPLOAD_CHUNK_SIZE = 256 * 1024;

  atomic_count s_uploads_multi_chunks_failed(0);

  void statistics_writer(ostream *o)
  {
    *o <<
      "gs file_transfer multi-part uploads:\n"
      "  chunks failed: " << s_uploads_multi_chunks_failed << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

file_transfer::file_transfer()
{
  _upload_chunk_size = 
    (config::get_upload_chunk_size() == -1)
      ? UPLOAD_CHUNK_SIZE
      : config::get_upload_chunk_size();
}

size_t file_transfer::get_upload_chunk_size()
{
  return _upload_chunk_size;
}

int file_transfer::upload_multi(const string &url, size_t size, const read_chunk_fn &on_read, string *returned_etag)
{
  return -ENOTSUP;
}
