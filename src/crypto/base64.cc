#include <stdexcept>

#ifdef __APPLE__
  #include <CoreFoundation/CoreFoundation.h>
  #include <Security/Security.h>
#else
  #include <openssl/bio.h>
  #include <openssl/buffer.h>
#endif

#include "crypto/base64.h"

using namespace std;

using namespace s3::crypto;

namespace
{
  #ifdef __APPLE__
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

      inline const T & get() const
      {
        return _ref;
      }

    private:
      T _ref;
    };
  #endif
}

string base64::encode(const uint8_t *input, size_t size)
{
  #ifdef __APPLE__
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
  #else
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
  #endif
}

void base64::decode(const string &input, vector<uint8_t> *output)
{
  #ifdef __APPLE__
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
  #else
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
  #endif
}
