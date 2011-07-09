#ifndef S3_AWS_SERVICE_IMPL_H
#define S3_AWS_SERVICE_IMPL_H

#include "service_impl.h"

namespace s3
{
  class request;

  class aws_service_impl : public service_impl
  {
  public:
    aws_service_impl();

    virtual const std::string & get_header_prefix();
    virtual const std::string & get_url_prefix();
    virtual const std::string & get_xml_namespace();

    virtual void sign(request *req);

  private:
    std::string _key, _secret;
  };
}

#endif
