#ifndef S3_SERVICE_IMPL_H
#define S3_SERVICE_IMPL_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  class request;

  class service_impl
  {
  public:
    typedef boost::shared_ptr<service_impl> ptr;

    virtual const std::string & get_header_prefix() = 0;
    virtual const std::string & get_url_prefix() = 0;
    virtual const std::string & get_xml_namespace() = 0;

    virtual void sign(request *req) = 0;
  };
}

#endif
