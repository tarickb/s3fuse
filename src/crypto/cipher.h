#ifndef S3_CRYPTO_CIPHER_H
#define S3_CRYPTO_CIPHER_H

#include <stdint.h>

#include <string>
#include <vector>

#include "crypto/encoder.h"

namespace s3
{
  namespace crypto
  {
    class symmetric_key;

    class cipher
    {
    public:
      template <class cipher_type>
      inline static void encrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size, 
        std::vector<uint8_t> *output)
      {
        cipher_type::encrypt(key, input, size, output);
      }

      template <class cipher_type, class encoder_type>
      inline static std::string encrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size)
      {
        std::vector<uint8_t> output;

        cipher_type::encrypt(key, input, size, &output);

        return encoder::encode<encoder_type>(output);
      }

      template <class cipher_type, class encoder_type>
      inline static std::string encrypt(
        const boost::shared_ptr<symmetric_key> &key,
        const std::string &input)
      {
        return encrypt<cipher_type, encoder_type>(key, reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
      }

      template <class cipher_type>
      inline static void decrypt(
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *input, 
        size_t size, 
        std::vector<uint8_t> *output)
      {
        cipher_type::decrypt(key, input, size, output);
      }

    };
  }
}

#endif
