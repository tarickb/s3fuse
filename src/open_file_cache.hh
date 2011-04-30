#ifndef S3_OPEN_FILE_CACHE_HH
#define S3_OPEN_FILE_CACHE_HH

#include <boost/thread.hpp>

#include "thread_pool.hh"

namespace s3
{
  class object;
  class thread_pool;

  class open_file_cache
  {
  public:
    open_file_cache(const boost::shared_ptr<thread_pool> &pool);

    inline int open(const boost::shared_ptr<object> &obj, uint64_t *handle)
    { 
      return _pool->call(boost::bind(&open_file_cache::__open, this, _1, obj, handle)); 
    }

    inline int close(uint64_t handle) { return flush(handle, true); }
    inline int flush(uint64_t handle) { return flush(handle, false); }

    int write(uint64_t handle, const char *buffer, size_t size, off_t offset);
    int read(uint64_t handle, char *buffer, size_t size, off_t offset);

  private:
    enum file_status
    {
      FS_NONE     = 0x0,
      FS_DIRTY    = 0x1,
      FS_FLUSHING = 0x2,
      FS_IN_USE   = 0x4
    };

    struct open_file
    {
      // TODO: move FILE * to this struct rather than object?
      boost::shared_ptr<object> obj;
      int status;

      open_file(const boost::shared_ptr<object> &obj_) : obj(obj_) { }
    };

    typedef boost::shared_ptr<open_file> open_file_ptr;
    typedef std::map<uint64_t, open_file_ptr> open_file_map;

    int flush(uint64_t handle, bool close_when_done);

    int __open(const request_ptr &req, const boost::shared_ptr<object> &obj, uint64_t *handle);
    int __flush(const request_ptr &req, const boost::shared_ptr<object> &obj);

    boost::mutex _mutex;
    open_file_map _files;
    boost::shared_ptr<thread_pool> _pool;
    uint64_t _next_handle;
  };
}

#endif
