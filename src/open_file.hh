#ifndef S3_OPEN_FILE_HH
#define S3_OPEN_FILE_HH

#include <stdint.h>
#include <sys/stat.h>

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class file_transfer;
  class mutexes;
  class object;
  class object_cache;

  class open_file
  {
  public:
    typedef boost::shared_ptr<open_file> ptr;

    ~open_file();

    inline int get_fd() { return _fd; }
    inline uint64_t get_handle() { return _handle; }

    int init();
    int cleanup();

    int truncate(off_t offset);
    int flush();
    int read(char *buffer, size_t size, off_t offset);
    int write(const char *buffer, size_t size, off_t offset);

  private:
    friend class object_cache; // for ctor, add_reference, release

    open_file(
      const boost::shared_ptr<mutexes> &mutexes, 
      const boost::shared_ptr<file_transfer> &file_transfer, 
      const boost::shared_ptr<object> &obj, 
      uint64_t handle);

    int add_reference(uint64_t *handle);
    bool release();

    boost::shared_ptr<mutexes> _mutexes;
    boost::shared_ptr<file_transfer> _file_transfer;
    boost::shared_ptr<object> _obj;
    uint64_t _handle, _ref_count;
    int _fd, _status, _error;
  };
}

#endif
