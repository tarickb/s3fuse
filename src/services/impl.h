/*
 * services/impl.h
 * -------------------------------------------------------------------------
 * Abstract base class for service-specific implementation classes.
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

#ifndef S3_SERVICES_IMPL_H
#define S3_SERVICES_IMPL_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace services
  {
    class file_transfer;

    class impl
    {
    public:
      typedef boost::shared_ptr<impl> ptr;

      virtual ~impl();

      virtual const std::string & get_header_prefix() = 0;
      virtual const std::string & get_header_meta_prefix() = 0;

      virtual const std::string & get_bucket_url() = 0;

      virtual std::string adjust_url(const std::string &url) = 0;
      virtual void pre_run(base::request *r, int iter) = 0;
      virtual bool should_retry(base::request *r, int iter);

      virtual boost::shared_ptr<file_transfer> build_file_transfer() = 0;
    };
  }
}

#endif
