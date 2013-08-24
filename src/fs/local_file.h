#ifndef S3_FS_LOCAL_FILE_H
#define S3_FS_LOCAL_FILE_H

#include <boost/shared_ptr.hpp>

namespace s3
{
  namespace fs
  {
    class local_file
    {
    public:
      typedef boost::shared_ptr<local_file> ptr;

      static void init();

      local_file(size_t size);
      ~local_file();

      inline int get_fd()
      {
        return _fd;
      }

      size_t get_size();
      void refresh_store_size();

    private:
      size_t _size;
      int _fd;
    };
  }
}

#endif
