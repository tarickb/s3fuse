#ifndef S3_GS_SERVICE_IMPL_H
#define S3_GS_SERVICE_IMPL_H

#include <boost/thread.hpp>

#include "service_impl.h"

namespace s3
{
  class request;

  class gs_service_impl : public service_impl
  {
  public:
    enum get_tokens_mode
    {
      GT_AUTH_CODE,
      GT_REFRESH
    };

    static const std::string & get_client_id();
    static const std::string & get_oauth_scope();
    static void get_tokens(get_tokens_mode mode, const std::string &key, std::string *access_token, time_t *expiry, std::string *refresh_token);

    gs_service_impl();

    virtual const std::string & get_header_prefix();
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
