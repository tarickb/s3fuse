#ifndef S3_CONFIG_H
#define S3_CONFIG_H

#include <string>

namespace s3
{
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
