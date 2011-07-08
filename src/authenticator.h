#ifndef S3_AUTHENTICATOR_H
#define S3_AUTHENTICATOR_H

namespace s3
{
  class request;

  class authenticator
  {
  public:
    virtual void sign(request *req) = 0;
  };
}

#endif
