#ifndef S3_OPENSSL_LOCKS_HH
#define S3_OPENSSL_LOCKS_HH

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
