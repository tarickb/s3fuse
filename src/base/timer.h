#ifndef S3_TIMER_H
#define S3_TIMER_H

#include <sys/time.h>

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
    };
  }
}

#endif
