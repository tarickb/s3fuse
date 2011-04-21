#ifndef S3_UTIL_HH
#define S3_UTIL_HH

#include <stdint.h>
#include <stdio.h>

#include <string>

namespace s3
{
  // TODO: remove "s3_" from other classes
  class util
  {
  public:
    static std::string base64_encode(const uint8_t *input, size_t size);
    static std::string sign(const std::string &key, const std::string &data);
    static std::string compute_md5_base64(FILE *f);
    static std::string url_encode(const std::string &url);
    static double get_current_time();
  };
}

#endif
