#ifndef S3_GS_AUTHENTICATOR_H
#define S3_GS_AUTHENTICATOR_H

#include <boost/thread.hpp>

#include "authenticator.h"

namespace s3
{
  class request;

  class gs_authenticator : public authenticator
  {
  public:
    enum get_tokens_mode
    {
      GT_AUTH_CODE,
      GT_REFRESH
    };

    static const std::string & get_client_id();
    static const std::string & get_oauth_scope();
    static void get_tokens(get_tokens_mode mode, const std::string &key, std::string *access_token, std::string *refresh_token, time_t *expiry);

    gs_authenticator();

    virtual const std::string & get_url_prefix();
    virtual const std::string & get_xml_namespace();

    virtual void sign(request *req);

  private:
    void refresh(const boost::mutex::scoped_lock &lock);

    boost::mutex _mutex;
    std::string _access_token, _refresh_token;
    time_t _expiry;
  };
}

#endif
