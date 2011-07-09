#ifndef S3_AUTHENTICATOR_H
#define S3_AUTHENTICATOR_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  class request;

  class authenticator
  {
  public:
    typedef boost::shared_ptr<authenticator> ptr;

    static ptr create(const std::string &service);

    virtual const std::string & get_url_prefix() = 0;
    virtual const std::string & get_xml_namespace() = 0;

    virtual void sign(request *req) = 0;
  };
}

#endif
