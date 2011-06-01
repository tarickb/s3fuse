#ifndef S3_OPENSSL_LOCKS_H
#define S3_OPENSSL_LOCKS_H

namespace s3
{
  class openssl_locks
  {
  public:
    static void init();
    static void release();
  };
}

#endif
