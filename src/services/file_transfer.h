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

#include <string>
#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

#include "base/request.h"

namespace s3
{
  namespace services
  {
    class file_transfer
    {
    public:
      typedef boost::function3<int, const char *, size_t, off_t> write_chunk_fn;
      typedef boost::function3<int, size_t, off_t, const base::char_vector_ptr &> read_chunk_fn;

      virtual ~file_transfer();

      virtual size_t get_download_chunk_size();
      virtual size_t get_upload_chunk_size();

      int download(const std::string &url, size_t size, const write_chunk_fn &on_write);
      int upload(const std::string &url, size_t size, const read_chunk_fn &on_read, std::string *returned_etag);

    protected:
      virtual int download_single(
        const base::request::ptr &req, 
        const std::string &url,
        size_t size,
        const write_chunk_fn &on_write);

      virtual int download_multi(
        const std::string &url,
        size_t size,
        const write_chunk_fn &on_write);

      virtual int upload_single(
        const base::request::ptr &req, 
        const std::string &url,
        size_t size,
        const read_chunk_fn &on_read,
        std::string *returned_etag);

      virtual int upload_multi(
        const std::string &url,
        size_t size,
        const read_chunk_fn &on_read,
        std::string *returned_etag);
    };
  }
}

#endif
