/*
 * service.h
 * -------------------------------------------------------------------------
 * Static methods for service-specific settings.
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

#ifndef S3_SERVICE_H
#define S3_SERVICE_H

#include <string>
#include <boost/smart_ptr.hpp>

#include "service_impl.h"

namespace s3
{
  class request;

  class service
  {
  public:
    static void init(const std::string &service);

    inline static const std::string & get_header_prefix() { return s_impl->get_header_prefix(); }
    inline static const std::string & get_url_prefix() { return s_impl->get_url_prefix(); }
    inline static const std::string & get_xml_namespace() { return s_impl->get_xml_namespace(); }

    inline static bool is_multipart_download_supported() { return s_impl->is_multipart_download_supported(); }
    inline static bool is_multipart_upload_supported() { return s_impl->is_multipart_upload_supported(); }

    // set last_sign_failed = true if the last sign() attempt failed.
    inline static void sign(request *req, bool last_sign_failed) { s_impl->sign(req, last_sign_failed); }

  private:
    static service_impl::ptr s_impl;
  };
}

#endif
