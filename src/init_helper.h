#ifndef S3_INIT_HELPER_H
#define S3_INIT_HELPER_H

#include <string>

namespace s3
{
  namespace services
  {
    class impl;
  }

  class init_helper
  {
  public:
    static services::impl * get_service_impl(std::string name = "");
  };
}

#endif
