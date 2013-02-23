/*
 * services/aws/impl.h
 * -------------------------------------------------------------------------
 * Service implementation for Amazon Web Services.
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

#ifndef S3_SERVICES_AWS_IMPL_H
#define S3_SERVICES_AWS_IMPL_H

#include "services/impl.h"

namespace s3
{
  namespace services
  {
    class file_transfer;

    namespace aws
    {
      class impl : public services::impl
      {
      public:
        impl();

        virtual ~impl() { }

        virtual const std::string & get_header_prefix();
        virtual const std::string & get_header_meta_prefix();

        virtual const std::string & get_directory_suffix();
        virtual const std::string & get_directory_url_suffix();

        virtual const std::string & get_bucket_url();

        virtual std::string adjust_url(const std::string &url);
        virtual void pre_run(base::request *r, int iter);

        virtual boost::shared_ptr<services::file_transfer> build_file_transfer();

      private:
        void sign(base::request *req);

        std::string _key, _secret, _endpoint, _bucket_url;
      };
    }
  }
}

#endif
