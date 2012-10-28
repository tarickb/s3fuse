#ifndef S3_CRYPTO_AES_CBC_256_H
#define S3_CRYPTO_AES_CBC_256_H

#include <stdint.h>

#ifdef __APPLE__
  #include <CommonCrypto/CommonCryptor.h>
#else
  #include <openssl/aes.h>
#endif

#include <stdexcept>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class cipher;
    class symmetric_key;

    class aes_cbc_256
    {
    public:
      #ifdef __APPLE__
        enum { BLOCK_LEN = kCCBlockSizeAES128 };
      #else
        enum { BLOCK_LEN = AES_BLOCK_SIZE };
      #endif

      enum { IV_LEN = BLOCK_LEN };
      enum { DEFAULT_KEY_LEN = 32 }; // 256 bits

    private:
      friend class cipher; // for encrypt() and decrypt()

      enum crypt_mode
      {
        M_ENCRYPT,
        M_DECRYPT
      };

      inline static void encrypt(const boost::shared_ptr<symmetric_key> &key, const uint8_t *in, size_t size, std::vector<uint8_t> *out)
      {
        crypt(M_ENCRYPT, key, in, size, out);
      }

      inline static void decrypt(const boost::shared_ptr<symmetric_key> &key, const uint8_t *in, size_t size, std::vector<uint8_t> *out)
      {
        crypt(M_DECRYPT, key, in, size, out);
      }

      static void crypt(
        crypt_mode mode, 
        const boost::shared_ptr<symmetric_key> &key, 
        const uint8_t *in, 
        size_t size, 
        std::vector<uint8_t> *out);
    };
  }
}

#endif
