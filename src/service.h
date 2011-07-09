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

    inline static void sign(request *req) { s_impl->sign(req); }

  private:
    static service_impl::ptr s_impl;
  };
}

#endif
