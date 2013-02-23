#ifndef S3_INIT_HELPER_H
#define S3_INIT_HELPER_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace services
  {
    class impl;
  }

  class init_helper
  {
  public:
    static boost::shared_ptr<services::impl> get_service_impl(std::string name = "");
  };
}

#endif
