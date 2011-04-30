#include "logging.hh"
#include "object.hh"
#include "open_file_cache.hh"
#include "thread_pool.hh"

using namespace s3;

open_file_cache::open_file_cache(const thread_pool::ptr &pool)
  : _pool(pool),
    _next_handle(0)
{
}

int open_file_cache::open(const object::ptr &obj, uint64_t *handle)
{
  return -EIO;
}

int open_file_cache::close(uint64_t handle)
{
  return -EIO;
}

int open_file_cache::flush(uint64_t handle)
{
  return -EIO;
}

int open_file_cache::write(uint64_t handle, const char *buffer, size_t size, off_t offset)
{
  return -EIO;
}

int open_file_cache::read(uint64_t handle, char *buffer, size_t size, off_t offset)
{
  return -EIO;
}
