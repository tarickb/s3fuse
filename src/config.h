#ifndef S3_CONFIG_H
#define S3_CONFIG_H

#include <string>
#include <limits>

namespace s3
{
  const uid_t UID_MAX = std::numeric_limits<uid_t>::max();
  const gid_t GID_MAX = std::numeric_limits<gid_t>::max();

  class config
  {
  public:
    static int init(const std::string &file = "");

    #define CONFIG(type, name, def) \
      private: \
        static type s_ ## name; \
      \
      public: \
        static const type & get_ ## name () { return s_ ## name; }

    #define CONFIG_REQUIRED(type, name, def) CONFIG(type, name, def)

    #include "config.inc"

    #undef CONFIG
    #undef CONFIG_REQUIRED
  };
}

#endif
