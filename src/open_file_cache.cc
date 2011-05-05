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

int open_file_cache::__open(const request::ptr &req, const object::ptr &obj, uint64_t *handle)
{
  FILE *temp_file;

  // TODO: check if file is already open, and return existing handle

  if (!obj)
    return -ENOENT;

  temp_file = tmpfile();

  if (!temp_file)
    return -errno;

  // TODO: run download in parallel?

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

int open_file_cache::flush(uint64_t handle, bool close_when_done)
{
  mutex::scoped_lock lock(_mutex);
  open_file_map::iterator itor = _files.find(handle);
  open_file_ptr file;
  int r = 0;

  if (itor == _files.end())
    return -EINVAL;

  file = itor->second;

  if (file->status & FS_IN_USE)
    return -EBUSY;

  if (file->status & FS_FLUSHING)
    return close_when_done ? -EBUSY : 0; // another thread is flushing this file, so don't report an error

  file->status |= FS_FLUSHING;
  lock.unlock();

  if (file->status & FS_DIRTY)
    r = _pool->call(bind(&open_file_cache::__flush, this, _1, file->obj));

  lock.lock();
  file->status &= ~FS_FLUSHING;

  if (!r) {
    file->status &= ~FS_DIRTY;

    if (close_when_done) {
      _files.erase(handle);

      file->obj->set_local_file(NULL);
      file->obj->invalidate();
    }
  }

  return r;
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

int open_file_cache::__flush(const request::ptr &req, const object::ptr &obj)
{
  struct stat s;
  FILE *f = obj->get_local_file();

  S3_DEBUG("open_file_cache::__flush", "file %s needs to be written.\n", obj->get_path().c_str());

  if (fflush(f))
    return -errno;

  if (fstat(fileno(f), &s))
    return -errno;

  S3_DEBUG("open_file_cache::__flush", "writing %zu bytes to path %s.\n", static_cast<size_t>(s.st_size), obj->get_path().c_str());

  rewind(f);

  // TODO: use multipart uploads?

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);
  req->set_header("Content-MD5", util::compute_md5_base64(f));
  req->set_input_file(f, s.st_size);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}
