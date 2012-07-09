/*
 * util_gnu.inc.cc
 * -------------------------------------------------------------------------
 * MD5 digests, various encodings (for GNU systems).
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <sys/time.h>

#include <stdexcept>

using namespace s3;
using namespace std;

string util::base64_encode(const uint8_t *input, size_t size)
{
  BIO *bio_b64 = BIO_new(BIO_f_base64());
  BIO *bio_mem = BIO_new(BIO_s_mem());
  BUF_MEM *mem;
  string ret;

  BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);

  bio_b64 = BIO_push(bio_b64, bio_mem);
  BIO_write(bio_b64, input, size);
  void(BIO_flush(bio_b64));
  BIO_get_mem_ptr(bio_b64, &mem);

  ret.resize(mem->length);
  memcpy(&ret[0], mem->data, mem->length);

  BIO_free_all(bio_b64);
  return ret;
}

void util::base64_decode(const string &input, vector<uint8_t> *output)
{
  const size_t READ_CHUNK = 1024;

  BIO *bio_b64 = BIO_new(BIO_f_base64());
  BIO *bio_input = BIO_new_mem_buf(const_cast<char *>(input.c_str()), -1);
  size_t total_bytes = 0;

  BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
  bio_b64 = BIO_push(bio_b64, bio_input);

  while (true) {
    int r;

    output->resize(total_bytes + READ_CHUNK);
    r = BIO_read(bio_b64, &(*output)[total_bytes], READ_CHUNK);

    if (r < 0) {
      BIO_free_all(bio_b64);
      throw runtime_error("failed while decoding base64.");
    }

    output->resize(total_bytes + r);
    total_bytes += r;

    if (r == 0)
      break;
  }

  BIO_free_all(bio_b64);
}

string util::sign(const string &key, const string &data)
{
  unsigned int hmac_sha1_len;
  uint8_t hmac_sha1[EVP_MAX_MD_SIZE];

  HMAC(
    EVP_sha1(), 
    reinterpret_cast<const void *>(key.data()), 
    key.size(), 
    reinterpret_cast<const uint8_t *>(data.data()), 
    data.size(), 
    hmac_sha1, 
    &hmac_sha1_len);

  return base64_encode(hmac_sha1, hmac_sha1_len);
}

string util::compute_md5(int fd, md5_output_type type, ssize_t size, off_t offset)
{
  const ssize_t BUF_LEN = 8 * 1024;

  EVP_MD_CTX md5_ctx;
  char buf[BUF_LEN];
  uint8_t md5_buf[EVP_MAX_MD_SIZE];
  unsigned int md5_len = 0;

  if (size == 0 && offset == 0)
    size = -1;

  EVP_DigestInit(&md5_ctx, EVP_md5());

  while (true) {
    ssize_t read_bytes, read_count;

    read_bytes = (size < 0 || size > BUF_LEN) ? BUF_LEN : size;
    read_count = pread(fd, buf, read_bytes, offset);

    if (read_count == -1)
      throw runtime_error("error while computing md5, in pread().");

    EVP_DigestUpdate(&md5_ctx, buf, read_count);

    offset += read_count;
    size -= read_count;

    if (read_count < BUF_LEN)
      break;
  }

  EVP_DigestFinal(&md5_ctx, md5_buf, &md5_len);

  if (type == MOT_BASE64)
    return base64_encode(md5_buf, md5_len);
  else if (type == MOT_HEX)
    return "\"" + hex_encode(md5_buf, md5_len) + "\"";
  else
    throw runtime_error("unknown md5 output type.");
}
