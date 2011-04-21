#ifndef S3_REQUEST_CACHE_HH
#define S3_REQUEST_CACHE_HH

#include <vector>
#include <boost/thread.hpp>

#include "s3_request.hh"

namespace s3
{
  class request_cache
  {
  public:
    request_cache();
    ~request_cache();

    request_ptr get();

  private:
    typedef std::vector<request *> request_vector;

    boost::mutex _mutex;
    request_vector _cache;
  };
}

#endif
