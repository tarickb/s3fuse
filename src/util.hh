#ifndef S3_UTIL_HH
#define S3_UTIL_HH

#include <stdint.h>
#include <stdio.h>

#include <string>

namespace s3
{
  enum md5_output_type
  {
    MOT_BASE64,
    MOT_HEX
  };

  class util
  {
  public:
    static std::string base64_encode(const uint8_t *input, size_t size);
    static std::string sign(const std::string &key, const std::string &data);
    static std::string compute_md5(int fd, off_t offset = 0, ssize_t size = 0, md5_output_type type = MOT_BASE64);
    static std::string url_encode(const std::string &url);
    static std::string hex_encode(const uint8_t *input, size_t size);
    static double get_current_time();
  };
}

#endif
