#ifndef S3_FS_BUCKET_VOLUME_KEY_H
#define S3_FS_BUCKET_VOLUME_KEY_H

#include <boost/smart_ptr.hpp>

#include "crypto/aes_cbc_256.h"

namespace s3
{
  namespace crypto
  {
    class buffer;
  }

  namespace fs
  {
    class bucket_volume_key
    {
    public:
      typedef crypto::aes_cbc_256_with_pkcs key_cipher;

      static bool is_present();

      static boost::shared_ptr<crypto::buffer> read(const boost::shared_ptr<crypto::buffer> &key);

      static void generate(const boost::shared_ptr<crypto::buffer> &key);
      static void reencrypt(const boost::shared_ptr<crypto::buffer> &old_key, const boost::shared_ptr<crypto::buffer> &new_key);
      static void remove();
    };
  }
}

#endif
