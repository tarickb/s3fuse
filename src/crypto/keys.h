#ifndef S3_CRYPTO_KEYS_H
#define S3_CRYPTO_KEYS_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class buffer;

    class keys
    {
    public:
      static void init(const std::string &key_file);

      static boost::shared_ptr<buffer> get_volume_key();
    };
  }
}

#endif
