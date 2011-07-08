#ifndef S3_AWS_AUTHENTICATOR_H
#define S3_AWS_AUTHENTICATOR_H

#include "authenticator.h"

namespace s3
{
  class request;

  class aws_authenticator : public authenticator
  {
  public:
    virtual void sign(request *req);
  };
}

#endif
