#ifndef S3_TIMER_H
#define S3_TIMER_H

#include <sys/time.h>

#include <string>

namespace s3
{
  namespace base
  {
    class timer
    {
    public:
      inline static double get_current_time()
      {
        timeval t;

        gettimeofday(&t, NULL);

        return double(t.tv_sec) + double(t.tv_usec) / 1.0e6;
      }

      inline static std::string get_http_time()
      {
        time_t sys_time;
        tm gm_time;
        char time_str[128];

        time(&sys_time);
        gmtime_r(&sys_time, &gm_time);
        strftime(time_str, 128, "%a, %d %b %Y %H:%M:%S GMT", &gm_time);

        return time_str;
      }
    };
  }
}

#endif
