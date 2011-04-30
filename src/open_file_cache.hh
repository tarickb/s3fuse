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

    inline int open(const boost::shared_ptr<object> &obj, uint64_t *handle) { ASYNC_CALL(_pool, open_file_cache, open, obj, handle); }
    inline int close(uint64_t handle) { ASYNC_CALL(_pool, open_file_cache, close, handle); }
    inline int flush(uint64_t handle) { ASYNC_CALL(_pool, open_file_cache, flush, handle); }

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

    ASYNC_DECL(open, const boost::shared_ptr<object> &obj, uint64_t *handle);
    ASYNC_DECL(close, uint64_t handle);
    ASYNC_DECL(flush, uint64_t handle);

    boost::mutex _mutex;
    open_file_map _files;
    boost::shared_ptr<thread_pool> _pool;
    uint64_t _next_handle;
  };
}

#endif
