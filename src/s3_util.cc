#include <string.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <sys/time.h>

#include <stdexcept>

#include "s3_util.hh"

using namespace s3;
using namespace std;

string util::base64_encode(const uint8_t *input, size_t size)
{
  BIO *bio_b64 = BIO_new(BIO_f_base64());
  BIO *bio_mem = BIO_new(BIO_s_mem());
  BUF_MEM *mem;
  string ret;

  bio_b64 = BIO_push(bio_b64, bio_mem);
  BIO_write(bio_b64, input, size);
  void(BIO_flush(bio_b64));
  BIO_get_mem_ptr(bio_b64, &mem);

  ret.resize(mem->length - 1);
  memcpy(&ret[0], mem->data, mem->length - 1);

  BIO_free_all(bio_b64);
  return ret;
}

string util::sign(const std::string &key, const std::string &data)
{
  unsigned int hmac_md5_len;
  uint8_t hmac_md5[EVP_MAX_MD_SIZE];

  HMAC(
    EVP_sha1(), 
    reinterpret_cast<const void *>(key.data()), 
    key.size(), 
    reinterpret_cast<const uint8_t *>(data.data()), 
    data.size(), 
    hmac_md5, 
    &hmac_md5_len);

  return base64_encode(hmac_md5, hmac_md5_len);
}

string util::compute_md5_base64(FILE *f)
{
  const size_t buf_len = 8 * 1024;

  EVP_MD_CTX md5_ctx;
  char buf[buf_len];
  uint8_t md5_buf[EVP_MAX_MD_SIZE];
  unsigned int md5_len = 0;
  size_t read_count = 0;

  EVP_DigestInit(&md5_ctx, EVP_md5());
  fseek(f, 0, SEEK_SET);

  while (true) {
    read_count = fread(buf, 1, buf_len, f);
    EVP_DigestUpdate(&md5_ctx, buf, read_count);

    if (read_count < buf_len)
      break;
  }

  if (!feof(f))
    throw runtime_error("error while computing md5.");

  EVP_DigestFinal(&md5_ctx, md5_buf, &md5_len);

  fseek(f, 0, SEEK_SET);

  return base64_encode(md5_buf, md5_len);
}

string util::url_encode(const std::string &url)
{
  const char *hex = "0123456789ABCDEF";
  string ret;

  ret.reserve(url.length());

  for (size_t i = 0; i < url.length(); i++) {
    if (url[i] == '/' || url[i] == '.' || url[i] == '-' || url[i] == '*' || url[i] == '_' || isalnum(url[i]))
      ret += url[i];
    else if (url[i] == ' ')
      ret += '+';
    else {
      ret += '%';
      ret += hex[static_cast<uint8_t>(url[i]) / 16];
      ret += hex[static_cast<uint8_t>(url[i]) % 16];
    }
  }

  return ret;
}

double util::get_current_time()
{
  timeval t;

  gettimeofday(&t, NULL);

  return double(t.tv_sec) + double(t.tv_usec) / 1.0e6;
}
