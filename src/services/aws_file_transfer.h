/*
 * services/aws_file_transfer.h
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

namespace s3
{
  namespace services
  {
    class aws_file_transfer : public file_transfer
    {
    public:
      aws_file_transfer();

      virtual size_t get_upload_chunk_size();

    protected:
      virtual int upload_multi(
        const std::string &url, 
        size_t size, 
        const read_chunk_fn &on_read, 
        std::string *returned_etag);

    private:
      struct upload_range
      {
        int id;
        size_t size;
        off_t offset;
        std::string etag;
      };

      int upload_part(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id, 
        const read_chunk_fn &on_read, 
        upload_range *range, 
        bool is_retry);

      int upload_multi_init(
        const base::request::ptr &req, 
        const std::string &url, 
        std::string *upload_id);

      int upload_multi_cancel(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id);

      int upload_multi_complete(
        const base::request::ptr &req, 
        const std::string &url, 
        const std::string &upload_id, 
        const std::string &upload_metadata, 
        std::string *etag);

      size_t _upload_chunk_size;
    };
  }
}

#endif
