#include <stdexcept>

#ifdef __APPLE__
  #include <CommonCrypto/CommonDigest.h>
#else
  #include <openssl/md5.h>
#endif

#include "crypto/md5.h"

using namespace std;

using namespace s3::crypto;

void md5::compute(const uint8_t *input, size_t size, uint8_t *hash)
{
  #ifdef __APPLE__
    CC_MD5(input, size, hash);
  #else
    MD5(input, size, hash);
  #endif
}

void md5::compute(int fd, uint8_t *hash)
{
  const ssize_t BUF_LEN = 8 * 1024;
  off_t offset = 0;
  char buf[BUF_LEN];

  #ifdef __APPLE__
    CC_MD5_CTX md5_ctx;

    CC_MD5_Init(&md5_ctx);
  #else
    EVP_MD_CTX md5_ctx;

    EVP_DigestInit(&md5_ctx, EVP_md5());
  #endif

  while (true) {
    ssize_t read_count;

    read_count = pread(fd, buf, BUF_LEN, offset);

    if (read_count == -1)
      throw runtime_error("error while computing md5, in pread().");

    #ifdef __APPLE__
      CC_MD5_Update(&md5_ctx, buf, read_count);
    #else
      EVP_DigestUpdate(&md5_ctx, buf, read_count);
    #endif

    offset += read_count;

    if (read_count < BUF_LEN)
      break;
  }

  #ifdef __APPLE__
    CC_MD5_Final(hash, &md5_ctx);
  #else
    EVP_DigestFinal(&md5_ctx, hash, NULL);
  #endif
}
