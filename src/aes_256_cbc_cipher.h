#ifndef S3_AES_256_CBC_CIPHER_H
#define S3_AES_256_CBC_CIPHER_H

#include <openssl/evp.h>

#include "cipher.h"

namespace s3
{
  class aes_256_cbc_cipher : public cipher
  {
  public:
    aes_256_cbc_cipher(const uint8_t *key, const std::string &iv);
    ~aes_256_cbc_cipher();

    virtual void encrypt(int in_fd, int out_fd);
    virtual void decrypt(int in_fd, int out_fd);

    virtual const std::string & get_iv();

  private:
    EVP_CIPHER_CTX _enc, _dec;
    std::vector<uint8_t> _iv;
    std::string _iv_str;
  };
}

#endif
