#ifndef S3_FS_LOCAL_FILE_STORE_H
#define S3_FS_LOCAL_FILE_STORE_H

namespace s3
{
  namespace fs
  {
    class local_file;

    class local_file_store
    {
    public:
      static void init();
      static void terminate();

    private:
      friend class local_file;

      static void increment(size_t size);
      static void decrement(size_t size);
    };
  }
}

#endif
