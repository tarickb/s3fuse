#ifndef S3_LOGGING_HH
#define S3_LOGGING_HH

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

namespace s3
{
  class logger
  {
  public:
    static void init(int min_level);

    inline void log(int level, const char *message, ...)
    {
      va_list args;

      va_start(args, message);

      if (level >= s_min_level)
        vfprintf(stderr, message, args);

      va_end(args);
    }

  private:
    static int s_min_level;
  };
}

// "##" between "," and "__VA_ARGS__" removes the comma if no arguments are provided
#define S3_DEBUG(fn, str, ...) fprintf(stderr, fn " [%lx]: " str, pthread_self(), ## __VA_ARGS__)
#define S3_ERROR(fn, str, ...) fprintf(stderr, fn ": " str, ## __VA_ARGS__)

#define S3_LOG(level, fn, str, ...) do { s3::logger::log(level, fn ": " str, ## __VA_ARGS__) } while (0)

#endif
