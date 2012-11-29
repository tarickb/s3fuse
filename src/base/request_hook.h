#ifndef S3_BASE_REQUEST_HOOK_H
#define S3_BASE_REQUEST_HOOK_H

#include <string>

namespace s3
{
  namespace base
  {
    class request;

    class request_hook
    {
    public:
      virtual std::string adjust_url(const std::string &url) = 0;
      virtual void pre_run(request *req, int iter) = 0;
      virtual bool should_retry(request *req, int iter) = 0;
    };
  }
}

#endif
