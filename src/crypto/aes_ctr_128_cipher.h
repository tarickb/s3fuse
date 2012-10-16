#ifndef S3_CRYPTO_AES_CTR_128_CIPHER_H
#define S3_CRYPTO_AES_CTR_128_CIPHER_H

#include <stdint.h>

#ifdef __APPLE__
  #include <CommonCrypto/CommonCryptor.h>
#else
  #include <openssl/aes.h>
#endif

#include <stdexcept>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class cipher_state;

    class aes_ctr_128_cipher
    {
    public:
      typedef boost::shared_ptr<aes_ctr_128_cipher> ptr;

      #ifdef __APPLE__
        enum { BLOCK_LEN = kCCBlockSizeAES128 };
      #else
        enum { BLOCK_LEN = AES_BLOCK_SIZE };
      #endif

      enum { IV_LEN = BLOCK_LEN / 2 };
      enum { KEY_LEN = BLOCK_LEN };

      aes_ctr_128_cipher(const boost::shared_ptr<cipher_state> &state, uint64_t starting_block);
      ~aes_ctr_128_cipher();

      // TODO: modify this to use an interface similar to encoder/hash/etc.
      inline void encrypt(const uint8_t *in, size_t size, uint8_t *out)
      {
        #ifdef __APPLE__
          CCCryptorStatus r;

          // TODO: find some way to force block boundary

          r = CCCryptorUpdate(
            _cryptor,
            in,
            size,
            out,
            size,
            NULL);

          if (r != kCCSuccess)
            throw std::runtime_error("CCCryptorUpdate() failed");
        #else
          // by always setting _num = 0 before encrypting we enforce our 
          // requirement that all encryption be started on block boundaries

          _num = 0;
          AES_ctr128_encrypt(in, out, size, &_key, _iv, _ecount_buf, &_num);
        #endif
      }

      inline void decrypt(const uint8_t *in, size_t size, uint8_t *out)
      {
        encrypt(in, size, out);
      }

    private:
      #ifdef __APPLE__
        CCCryptorRef _cryptor;
      #else
        AES_KEY _key;
        uint8_t _iv[BLOCK_LEN];
        uint8_t _ecount_buf[BLOCK_LEN];
        unsigned int _num;
      #endif
    };
  }
}

#endif
