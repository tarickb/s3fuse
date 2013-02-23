/*
 * services/service.h
 * -------------------------------------------------------------------------
 * Static methods for service-specific settings.
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

#ifndef S3_SERVICES_SERVICE_H
#define S3_SERVICES_SERVICE_H

#include <string>
#include <boost/smart_ptr.hpp>

#include "services/impl.h"

namespace s3
{
  namespace base
  {
    class request_hook;
  }

  namespace services
  {
    class service
    {
    public:
      static void init(const impl::ptr &svc);

      inline static const std::string & get_header_prefix() { return s_impl->get_header_prefix(); }
      inline static const std::string & get_header_meta_prefix() { return s_impl->get_header_meta_prefix(); }

      inline static const std::string & get_bucket_url() { return s_impl->get_bucket_url(); }

      inline static base::request_hook * get_request_hook() { return s_hook.get(); }

      inline static const boost::shared_ptr<file_transfer> & get_file_transfer() { return s_file_transfer; }

    private:
      static impl::ptr s_impl;
      static boost::shared_ptr<base::request_hook> s_hook;
      static boost::shared_ptr<file_transfer> s_file_transfer;
    };
  }
}

#endif
