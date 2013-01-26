#ifndef S3_BASE_PATHS_H
#define S3_BASE_PATHS_H

#include <string>

namespace s3
{
  namespace base
  {
    class paths
    {
    public:
      static std::string transform(const std::string &path);
    };
  }
}

#endif
