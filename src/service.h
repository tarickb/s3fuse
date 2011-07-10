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

    // allow calls to succeed if s_impl is NULL because service::* methods are
    // used during service_impl class initialization.

    inline static const std::string & get_header_prefix() { return s_impl ? s_impl->get_header_prefix() : s_empty_string; }
    inline static const std::string & get_url_prefix() { return s_impl ? s_impl->get_url_prefix() : s_empty_string; }
    inline static const std::string & get_xml_namespace() { return s_impl ? s_impl->get_xml_namespace() : s_empty_string; }

    inline static bool is_multipart_download_supported() { return s_impl ? s_impl->is_multipart_download_supported() : false; }
    inline static bool is_multipart_upload_supported() { return s_impl ? s_impl->is_multipart_upload_supported() : false; }

    inline static void sign(request *req)
    {
      if (s_impl)
        s_impl->sign(req); 
    }

  private:
    static service_impl::ptr s_impl;
    static std::string s_empty_string;
  };
}

#endif
