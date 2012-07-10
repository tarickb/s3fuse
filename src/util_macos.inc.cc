/*
 * util_macos.inc.cc
 * -------------------------------------------------------------------------
 * MD5 digests, various encodings (for Mac OS).
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

#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <string.h>
#include <sys/time.h>

#include <stdexcept>

using namespace s3;
using namespace std;

template <typename T>
class cf_wrapper
{
public:
  inline cf_wrapper()
    : _ref(NULL)
  {
  }

  inline ~cf_wrapper()
  {
    if (_ref)
      CFRelease(_ref);
  }

  inline void set(const T &ref)
  {
    if (_ref)
      CFRelease(_ref);

    _ref = ref;
  }

  inline const T & get()
  {
    return _ref;
  }

private:
  T _ref;
};

string util::base64_encode(const uint8_t *input, size_t size)
{
  cf_wrapper<SecTransformRef> enc;
  cf_wrapper<CFDataRef> in, out;
  size_t out_len = 0;
  CFErrorRef err = NULL;
  string ret;

  in.set(CFDataCreate(NULL, input, size));
  enc.set(SecEncodeTransformCreate(kSecBase64Encoding, &err));

  if (err)
    throw runtime_error("error creating base64 encoder.");

  SecTransformSetAttribute(enc.get(), kSecTransformInputAttributeName, in.get(), &err);

  if (err)
    throw runtime_error("error setting encoder input.");

  out.set(static_cast<CFDataRef>(SecTransformExecute(enc.get(), &err)));

  if (err)
    throw runtime_error("error executing base64 encoder.");

  out_len = CFDataGetLength(out.get());
  ret.resize(out_len);
  memcpy(&ret[0], CFDataGetBytePtr(out.get()), out_len);

  return ret;
}

void util::base64_decode(const string &input, vector<uint8_t> *output)
{
  cf_wrapper<SecTransformRef> dec;
  cf_wrapper<CFDataRef> in, out;
  size_t out_len = 0;
  CFErrorRef err = NULL;

  in.set(CFDataCreate(NULL, reinterpret_cast<const uint8_t *>(input.c_str()), input.size()));
  dec.set(SecDecodeTransformCreate(kSecBase64Encoding, &err));

  if (err)
    throw runtime_error("error creating base64 decoder.");

  SecTransformSetAttribute(dec.get(), kSecTransformInputAttributeName, in.get(), &err);

  if (err)
    throw runtime_error("error setting decoder input.");

  out.set(static_cast<CFDataRef>(SecTransformExecute(dec.get(), &err)));

  if (err)
    throw runtime_error("error executing base64 decoder.");

  out_len = CFDataGetLength(out.get());
  output->resize(out_len);
  memcpy(&(*output)[0], CFDataGetBytePtr(out.get()), out_len);
}

string util::sign(const string &key, const string &data)
{
  const size_t SHA1_SIZE = 20; // 160 bits

  uint8_t hmac_sha1[SHA1_SIZE];

  CCHmac(
    kCCHmacAlgSHA1,
    reinterpret_cast<const void *>(key.data()),
    key.size(),
    reinterpret_cast<const void *>(data.data()),
    data.size(),
    hmac_sha1);

  return base64_encode(hmac_sha1, SHA1_SIZE);
}

string util::compute_md5(int fd, md5_output_type type, ssize_t size, off_t offset)
{
  const ssize_t BUF_LEN = 8 * 1024;

  CC_MD5_CTX md5_ctx;
  char buf[BUF_LEN];
  uint8_t md5_buf[CC_MD5_DIGEST_LENGTH];

  if (size == 0 && offset == 0)
    size = -1;

  CC_MD5_Init(&md5_ctx);

  while (true) {
    ssize_t read_bytes, read_count;

    read_bytes = (size < 0 || size > BUF_LEN) ? BUF_LEN: size;
    read_count = pread(fd, buf, read_bytes, offset);

    if (read_count == -1)
      throw runtime_error("error while computing md5, in pread().");

    CC_MD5_Update(&md5_ctx, buf, read_count);

    offset += read_count;
    size -= read_count;

    if (read_count < BUF_LEN)
      break;
  }

  CC_MD5_Final(md5_buf, &md5_ctx);

  if (type == MOT_BASE64)
    return base64_encode(md5_buf, CC_MD5_DIGEST_LENGTH);
  else if (type == MOT_HEX)
    return "\"" + hex_encode(md5_buf, CC_MD5_DIGEST_LENGTH) + "\"";
  else if (type == MOT_HEX_NO_QUOTE)
    return hex_encode(md5_buf, CC_MD5_DIGEST_LENGTH);
  else
    throw runtime_error("unknown md5 output type.");
}

string util::compute_md5(const uint8_t *input, size_t size, md5_output_type type)
{
  uint8_t md5_buf[CC_MD5_DIGEST_LENGTH];

  CC_MD5(input, size, md5_buf);

  if (type == MOT_BASE64)
    return base64_encode(md5_buf, CC_MD5_DIGEST_LENGTH);
  else if (type == MOT_HEX)
    return "\"" + hex_encode(md5_buf, CC_MD5_DIGEST_LENGTH) + "\"";
  else if (type == MOT_HEX_NO_QUOTE)
    return hex_encode(md5_buf, CC_MD5_DIGEST_LENGTH);
  else
    throw runtime_error("unknown md5 output type.");
}
