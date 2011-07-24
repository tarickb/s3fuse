#ifndef S3_CIPHER_FACTORY_H
#define S3_CIPHER_FACTORY_H

#include <string>
#include <vector>
#include <boost/function.hpp>

namespace s3
{
  class cipher;

  class cipher_factory
  {
  public:
    static void init(const std::string &cipher, const std::string &key_file);

    inline static boost::shared_ptr<cipher> create(const std::string &iv = "")
    {
      return s_cipher_ctor(&s_key[0], iv);
    }

  private:
    typedef boost::function2<cipher::ptr, const uint8_t *, const std::string &> ctor_fn;

    static ctor_fn s_cipher_ctor;
    static std::vector<uint8_t> s_key;
  };
}

#endif
