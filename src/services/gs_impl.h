/*
 * services/gs_impl.h
 * -------------------------------------------------------------------------
 * Service implementation for Google Storage for Developers.
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

#ifndef S3_SERVICES_GS_IMPL_H
#define S3_SERVICES_GS_IMPL_H

#include <boost/thread.hpp>

#include "services/impl.h"

namespace s3
{
  namespace services
  {
    class gs_impl : public impl
    {
    public:
      enum get_tokens_mode
      {
        GT_AUTH_CODE,
        GT_REFRESH
      };

      static const std::string & get_new_token_url();
      static void get_tokens(get_tokens_mode mode, const std::string &key, std::string *access_token, time_t *expiry, std::string *refresh_token);

      static void write_token(const std::string &file, const std::string &token);
      static std::string read_token(const std::string &file);

      gs_impl();

      virtual ~gs_impl() { }

      virtual const std::string & get_header_prefix();
      virtual const std::string & get_header_meta_prefix();
      virtual const std::string & get_xml_namespace();

      virtual bool is_multipart_download_supported();
      virtual bool is_multipart_upload_supported();

      virtual const std::string & get_bucket_url();

      virtual std::string adjust_url(const std::string &url);
      virtual void pre_run(base::request *r, int iter);
      virtual bool should_retry(base::request *r, int iter);

    private:
      void sign(base::request *req, int iter);
      void refresh(const boost::mutex::scoped_lock &lock);

      boost::mutex _mutex;
      std::string _access_token, _refresh_token, _bucket_url;
      time_t _expiry;
    };
  }
}

#endif
