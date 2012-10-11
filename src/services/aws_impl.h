/*
 * aws_service_impl.h
 * -------------------------------------------------------------------------
 * Service implementation for Amazon Web Services.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#ifndef S3_SERVICES_AWS_IMPL_H
#define S3_SERVICES_AWS_IMPL_H

#include "services/impl.h"

namespace s3
{
  class request;

  namespace services
  {
    class aws_impl : public impl
    {
    public:
      aws_impl();

      virtual const std::string & get_header_prefix();
      virtual const std::string & get_header_meta_prefix();
      virtual const std::string & get_url_prefix();
      virtual const std::string & get_xml_namespace();

      virtual bool is_multipart_download_supported();
      virtual bool is_multipart_upload_supported();

      virtual const std::string & get_bucket_url();

      virtual const signing_function & get_signing_function();

    private:
      void sign(request *req, bool last_sign_failed);

      std::string _key, _secret, _endpoint, _bucket_url;
      signing_function _signing_function;
    };
  }
}

#endif
