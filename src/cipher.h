#ifndef S3_CIPHER_H
#define S3_CIPHER_H

#include <stdint.h>

#include <vector>
#include <boost/smart_ptr.hpp>

namespace s3
{
  class cipher
  {
  public:
    typedef boost::shared_ptr<cipher> ptr;

    virtual const std::string & get_iv() = 0;

    virtual void encrypt(int in_fd, int out_fd) = 0;
    virtual void decrypt(int in_fd, int out_fd) = 0;
  };
}

#endif
