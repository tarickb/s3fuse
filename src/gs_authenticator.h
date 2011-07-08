#ifndef S3_GS_AUTHENTICATOR_H
#define S3_GS_AUTHENTICATOR_H

#include <string>
#include <boost/thread.hpp>

#include "authenticator.h"

namespace s3
{
  class request;

  class gs_authenticator : public authenticator
  {
  public:
    static const std::string & get_client_id();
    static const std::string & get_scope();
    static void get_tokens(const std::string &code, std::string *access_token, std::string *refresh_token, time_t *expiry);

    gs_authenticator();

    virtual void sign(request *req);

  private:
    void refresh();

    boost::mutex _mutex;
    std::string _access_token, _refresh_token;
  };
}

#endif
