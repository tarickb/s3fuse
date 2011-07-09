#ifndef S3_AWS_AUTHENTICATOR_H
#define S3_AWS_AUTHENTICATOR_H

#include "authenticator.h"

namespace s3
{
  class request;

  class aws_authenticator : public authenticator
  {
  public:
    aws_authenticator();

    virtual const std::string & get_url_prefix();
    virtual const std::string & get_xml_namespace();

    virtual void sign(request *req);

  private:
    std::string _key, _secret;
  };
}

#endif
