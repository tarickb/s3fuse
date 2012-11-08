#ifndef S3_CRYPTO_PASSWORDS_H
#define S3_CRYPTO_PASSWORDS_H

#include <string>

namespace s3
{
  namespace crypto
  {
    class passwords
    {
    public:
      static std::string read_from_stdin(const std::string &prompt);
    };
  }
}

#endif
