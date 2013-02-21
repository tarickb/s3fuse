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
#include "services/multipart_transfer.h"

namespace s3
{
  namespace services
  {
    namespace gs
    {
      class file_transfer : public services::file_transfer
      {
      public:
        file_transfer();

        virtual size_t get_upload_chunk_size();

      protected:
        virtual int upload_multi(const std::string &url, size_t size, const read_chunk_fn &on_read, std::string *returned_etag);

      private:
        struct upload_range
        {
          size_t size;
          off_t offset;
        };

        int read_and_upload(
          const base::request::ptr &req,
          const std::string &url,
          const read_chunk_fn &on_read,
          upload_range *range,
          size_t total_size);

        int upload_part(
          const base::request::ptr &req, 
          const std::string &url, 
          const read_chunk_fn &on_read, 
          upload_range *range, 
          bool is_retry);

        int upload_last_part(
          const base::request::ptr &req, 
          const std::string &url, 
          const read_chunk_fn &on_read, 
          upload_range *range, 
          size_t total_size,
          std::string *returned_etag);

        int upload_multi_init(
          const base::request::ptr &req, 
          const std::string &url, 
          std::string *location);

        size_t _upload_chunk_size;
      };
    }
  }
}

#endif
