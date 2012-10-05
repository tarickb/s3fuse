#ifndef S3_SYMLINK_H
#define S3_SYMLINK_H

#include "object.h"
#include "thread_pool.h"

namespace s3
{
  class symlink : public object, public boost::enable_shared_from_this<symlink>
  {
  public:
    symlink(const std::string &path);
    virtual ~symlink();

    inline int read(std::string *target)
    {
      return thread_pool::call(thread_pool::PR_FG, bind(&symlink::read, shared_from_this(), _1, target));
    }

  private:
    int read(const boost::shared_ptr<request> &req, std::string *target);
  };
}

#endif
