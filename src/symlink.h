#ifndef S3_SYMLINK_H
#define S3_SYMLINK_H

#include "object.h"

namespace s3
{
  class symlink : public object
  {
  public:
    symlink(const std::string &path);
    virtual ~symlink();
  };
}

#endif
