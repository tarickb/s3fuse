/*
 * fs/encrypted_file.h
 * -------------------------------------------------------------------------
 * Represents an encrypted file object.
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

#ifndef S3_FS_ENCRYPTED_FILE_H
#define S3_FS_ENCRYPTED_FILE_H

#include <string>
#include <vector>

#include "fs/file.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace crypto
  {
    class symmetric_key;
  }

  namespace fs
  {
    class encrypted_file : public file
    {
    public:
      typedef boost::shared_ptr<encrypted_file> ptr;

      encrypted_file(const std::string &path);
      virtual ~encrypted_file();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<encrypted_file>(object::shared_from_this());
      }

    protected:
      virtual void init(const boost::shared_ptr<base::request> &req);

      virtual void set_request_headers(const boost::shared_ptr<base::request> &req);

      virtual int is_downloadable();

      virtual int prepare_upload();
      virtual int finalize_upload(const std::string &returned_etag);

      virtual int read_chunk(size_t size, off_t offset, std::vector<char> *buffer);
      virtual int write_chunk(const char *buffer, size_t size, off_t offset);

    private:
      boost::shared_ptr<crypto::symmetric_key> _meta_key, _data_key;
      std::string _enc_iv, _enc_meta;
    };
  }
}

#endif
