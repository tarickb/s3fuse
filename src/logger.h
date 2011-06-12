#ifndef S3_LOGGER_H
#define S3_LOGGER_H

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

namespace s3
{
  class logger
  {
  public:
    static void init(int max_level);

    inline static void log(int level, const char *message, ...)
    {
      va_list args;

      if (level <= s_max_level) {
        va_start(args, message);
        vfprintf(stderr, message, args);
        va_end(args);
      }

      // can't reuse va_list

      va_start(args, message);
      vsyslog(level, message, args);
      va_end(args);
    }

  private:
    static int s_max_level;
  };
}

// "##" between "," and "__VA_ARGS__" removes the comma if no arguments are provided
#define S3_LOG(level, fn, str, ...) do { s3::logger::log(level, fn ": " str, ## __VA_ARGS__); } while (0)

#endif
