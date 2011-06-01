#ifndef S3_UTIL_H
#define S3_UTIL_H

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
    static std::string compute_md5(int fd, md5_output_type type = MOT_BASE64, ssize_t size = 0, off_t offset = 0);
    static std::string url_encode(const std::string &url);
    static std::string hex_encode(const uint8_t *input, size_t size);
    static double get_current_time();
    static bool is_valid_md5(const std::string &md5);
  };
}

#endif
