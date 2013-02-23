#ifndef S3_INIT_H
#define S3_INIT_H

#include <string>

#include "base/logger.h"

namespace s3
{
  class init
  {
  public:
    enum base_flags
    {
      IB_NONE       = 0x0,
      IB_WITH_STATS = 0x1
    };

    static void base(int flags = IB_NONE, int verbosity = LOG_WARNING, const std::string &config_file = "");
    static void fs();
    static void services();
    static void threads();
  };
}

#endif
