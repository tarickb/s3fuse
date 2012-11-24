#ifndef S3_FS_ENCRYPTION_H
#define S3_FS_ENCRYPTION_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class buffer;
  }

  namespace fs
  {
    class encryption
    {
    public:
      static void init();

      static boost::shared_ptr<crypto::buffer> get_volume_key();
    };
  }
}

#endif
