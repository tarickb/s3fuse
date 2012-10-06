#ifndef S3_ENCRYPTED_FILE_H
#define S3_ENCRYPTED_FILE_H

#include "file.h"

namespace s3
{
  class encrypted_file : public file
  {
  public:
    typedef boost::shared_ptr<encrypted_file> ptr;

    encrypted_file(const std::string &path);
    virtual ~encrypted_file();

    inline ptr shared_from_this()
    {
      return boost::static_pointer_cast<encrypted_file>(object::shared_from_this());
    }
  };
}

#endif
