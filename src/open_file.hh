#ifndef S3_OPEN_FILE_HH
#define S3_OPEN_FILE_HH

#include <stdint.h>
#include <sys/stat.h>

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class object;
  class open_file_map;

  class open_file
  {
  public:
    typedef boost::shared_ptr<open_file> ptr;

    // TODO: move to private
    open_file(open_file_map *map, const boost::shared_ptr<object> &obj, uint64_t handle);
    ~open_file();

    // TODO: give object the fd, and let it call fstat()
    inline size_t get_size()
    {
      struct stat s;

      if (fstat(_fd, &s) == 0)
        return s.st_size;
      else
        return 0;
    }

    int add_reference(uint64_t *handle);
    bool release();

    int init();
    int cleanup();

    int flush();
    int read(char *buffer, size_t size, off_t offset);
    int write(const char *buffer, size_t size, off_t offset);

  private:
    open_file_map *_map;
    boost::shared_ptr<object> _obj;
    uint64_t _handle, _ref_count;
    int _fd, _status, _error;
  };
}

#endif
