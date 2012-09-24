#ifndef S3_SYMLINK_H
#define S3_SYMLINK_H

#include "object.h"

namespace s3
{
  class symlink : public object
  {
  public:
    static const std::string & get_default_content_type();

    symlink(const std::string &path);
    virtual ~symlink();
  };
}

#endif
