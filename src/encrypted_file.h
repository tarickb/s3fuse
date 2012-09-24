#ifndef S3_ENCRYPTED_FILE_H
#define S3_ENCRYPTED_FILE_H

#include "object.h"

namespace s3
{
  class encrypted_file : public object
  {
  public:
    static const std::string & get_default_content_type();

    encrypted_file(const std::string &path);
    virtual ~encrypted_file();
  };
}

#endif
