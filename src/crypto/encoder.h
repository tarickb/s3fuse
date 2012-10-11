#ifndef S3_CRYPTO_ENCODER_H
#define S3_CRYPTO_ENCODER_H

#include <string>
#include <vector>

namespace s3
{
  namespace crypto
  {
    class encoder
    {
    public:
      template <class encoder_type>
      inline static std::string encode(const uint8_t *input, size_t size)
      {
        return encoder_type::encode(input, size);
      }

      template <class encoder_type>
      inline static std::string encode(const char *input, size_t size)
      {
        return encoder_type::encode(reinterpret_cast<const uint8_t *>(input), size);
      }

      template <class encoder_type>
      inline static std::string encode(const std::string &input)
      {
        return encoder_type::encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
      }

      template <class encoder_type>
      inline static std::string encode(const std::vector<uint8_t> &input)
      {
        return encoder_type::encode(&input[0], input.size());
      }
    };
  }
}

#endif
