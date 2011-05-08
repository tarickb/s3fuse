#ifndef S3_OPEN_FILE_CACHE_HH
#define S3_OPEN_FILE_CACHE_HH

#include <boost/thread.hpp>

#include "thread_pool.hh"

namespace s3
{
  class file_transfer;
  class object;
  class open_file;
  class thread_pool;

  class open_file_cache
  {
  public:
    open_file_cache(const boost::shared_ptr<thread_pool> &pool);

    int open(const request_ptr &req, const boost::shared_ptr<object> &obj, uint64_t *handle);
    inline int close(const request_ptr &req, uint64_t handle) { return flush(req, handle, true); }
    inline int flush(const request_ptr &req, uint64_t handle) { return flush(req, handle, false); }

    int write(uint64_t handle, const char *buffer, size_t size, off_t offset);
    int read(uint64_t handle, char *buffer, size_t size, off_t offset);

  private:
    typedef boost::shared_ptr<open_file> open_file_ptr;
    typedef std::map<uint64_t, open_file_ptr> open_file_map;

    int flush(const request_ptr &req, uint64_t handle, bool close_when_done);

    boost::mutex _mutex;
    open_file_map _files;
    boost::shared_ptr<file_transfer> _ft;
    uint64_t _next_handle;
  };
}

#endif
