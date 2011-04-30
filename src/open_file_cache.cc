#include "logging.hh"
#include "object.hh"
#include "open_file_cache.hh"
#include "request.hh"
#include "thread_pool.hh"

using namespace boost;

using namespace s3;

open_file_cache::open_file_cache(const thread_pool::ptr &pool)
  : _pool(pool),
    _next_handle(0)
{
}

ASYNC_DEF(open_file_cache, open, const object::ptr &obj, uint64_t *handle)
{
  FILE *temp_file;

  if (!obj)
    return -ENOENT;

  temp_file = tmpfile();

  if (!temp_file)
    return -errno;

  req->init(HTTP_GET);
  req->set_url(obj->get_url());
  req->set_output_file(temp_file);

  req->run();

  if (req->get_response_code() != 200) {
    fclose(temp_file);
    return (req->get_response_code() == 404) ? -ENOENT : -EIO;
  }

  fflush(temp_file);

  {
    mutex::scoped_lock lock(_mutex);

    if (obj->get_local_file()) {
      fclose(temp_file);
      return -EBUSY;
    }

    obj->set_local_file(temp_file);

    *handle = _next_handle++;
    _files[*handle].reset(new open_file(obj));
  }

  S3_DEBUG("open_file_cache::open", "opened file %s with handle %" PRIu64 ".\n", obj->get_path().c_str(), *handle);

  return 0;
}

ASYNC_DEF(open_file_cache, close, uint64_t handle)
{
  return -EIO;
}

ASYNC_DEF(open_file_cache, flush, uint64_t handle)
{
  return -EIO;
}

int open_file_cache::write(uint64_t handle, const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_FLUSHING)
    return -EBUSY;

  file->status |= FS_IN_USE;
  lock.unlock();

  r = pwrite(fileno(file->obj->get_local_file()), buffer, size, offset);

  lock.lock();
  file->status &= ~FS_IN_USE;
  file->status |= FS_DIRTY;

  return r;
}

int open_file_cache::read(uint64_t handle, char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_FLUSHING)
    return -EBUSY;

  file->status |= FS_IN_USE;
  lock.unlock();

  r = pread(fileno(file->obj->get_local_file()), buffer, size, offset);

  lock.lock();
  file->status &= ~FS_IN_USE;

  return r;
}
