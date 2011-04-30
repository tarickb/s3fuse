#ifndef S3_OPEN_FILE_CACHE_HH
#define S3_OPEN_FILE_CACHE_HH

#include <boost/thread.hpp>

// TODO: remove
/*

object::get_open_file() -> open_file::ptr

open_file_cache::get(string path)
open_file_cache::get(uint64_t context)

minimize locking:

lock on:
  - open file
  - close file
  - write/mark dirty
  - state transitions

file does:
  - copy to tmp
  - copy from tmp
  - read/write

notes:
- create an open_file object
  - contains FILE *, uint64_t context (probably just "this")
  - object::set_open_file() (can be NULL)
- open_file_cache maps uint64_t to object
  - interface for read, write, open, close, flush
  - using its own lock (per-file?)

open_file_cache _cache;

*/

namespace s3
{
  class object;
  class thread_pool;

  class open_file_cache
  {
  public:
    open_file_cache(const boost::shared_ptr<thread_pool> &pool);

    int open(const boost::shared_ptr<object> &obj, uint64_t *handle);
    int close(uint64_t handle);

    int write(uint64_t handle, const char *buffer, size_t size, off_t offset);
    int read(uint64_t handle, char *buffer, size_t size, off_t offset);

    int flush(uint64_t handle);

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
    };

    typedef std::map<uint64_t, open_file> open_file_map;

    boost::mutex _mutex;
    open_file_map _map;
    boost::shared_ptr<thread_pool> _pool;
    uint64_t _next_handle;
  };
}

#endif
