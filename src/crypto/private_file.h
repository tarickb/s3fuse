#ifndef S3_CRYPTO_PRIVATE_FILE_H
#define S3_CRYPTO_PRIVATE_FILE_H

#include <fstream>
#include <string>

namespace s3
{
  namespace crypto
  {
    class private_file
    {
    public:
      static void open(const std::string &file, std::ifstream *f);
      static void open(const std::string &file, std::ofstream *f);
    };
  }
}

#endif
