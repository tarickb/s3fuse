#ifndef S3_OPEN_FILE_HH
#define S3_OPEN_FILE_HH

#include <stdint.h>
#include <sys/stat.h>

#include <string>
#include <boost/shared_ptr.hpp>

namespace s3
{
  class object;

  class open_file
  {
  public:
    typedef boost::shared_ptr<open_file> ptr;

    open_file(const boost::shared_ptr<object> &obj, const std::string &temp_name, int fd, uint64_t handle);
    ~open_file();

    inline size_t get_size()
    {
      struct stat s;

      if (fstat(_fd, &s) == 0)
        return s.st_size;
      else
        return 0;
    }

    inline int get_fd() { return _fd; }

    int clone_handle(uint64_t *handle);

  private:
    boost::shared_ptr<object> _obj;
    std::string _temp_name;
    int _fd;
    uint64_t _handle;
  };
}

#endif
