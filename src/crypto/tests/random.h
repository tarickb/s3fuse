#ifndef S3_CRYPTO_TESTS_RANDOM_H
#define S3_CRYPTO_TESTS_RANDOM_H

#include <stdexcept>
#include <fstream>
#include <vector>

namespace s3
{
  namespace crypto
  {
    namespace tests
    {
      class random
      {
      public:
        inline static void read(size_t size, std::vector<uint8_t> *out)
        {
          std::ifstream f("/dev/urandom", std::ifstream::in);

          if (!f.good())
            throw std::runtime_error("unable to open /dev/urandom");

          out->resize(size);

          f.read(reinterpret_cast<char *>(&(*out)[0]), size);

          if (static_cast<size_t>(f.gcount()) != size)
            throw std::runtime_error("failed to read requested number of bytes");
        }
      };
    }
  }
}

#endif
