#ifndef S3_FS_LOCAL_FILE_STORE_H
#define S3_FS_LOCAL_FILE_STORE_H

#include <string>

namespace s3
{
  namespace fs
  {
    class local_file_store
    {
    public:
      static void init();
      static void terminate();

      static void increment_store_size(size_t size);
      static void decrement_store_size(size_t size);

      static const std::string & get_temp_file_template();
    };
  }
}

#endif
