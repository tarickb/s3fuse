#include <list>
#include <boost/lexical_cast.hpp>
#include <pugixml/pugixml.hpp>

#include "file_transfer.hh"
#include "logging.hh"
#include "object.hh"
#include "open_file_cache.hh"
#include "request.hh"
#include "thread_pool.hh"

using namespace boost;
using namespace pugi;
using namespace std;

using namespace s3;

namespace s3
{
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
    object::ptr obj;
    string temp_name;
    int status;
    int ref_count;

    open_file(const boost::shared_ptr<object> &obj_) : obj(obj_) { }
  };
}

open_file_cache::open_file_cache(const thread_pool::ptr &pool)
  : _ft(new file_transfer(pool)),
    _next_handle(0)
{
}

int open_file_cache::open(const request::ptr &req, const object::ptr &obj, uint64_t *handle)
{
  mutex::scoped_lock lock(_mutex);
  open_file::ptr file;
  size_t size;
  int r;

  file = obj->get_open_file();

  if (file)
    return file->clone_handle(handle);

  file.reset(new open_file(obj, _next_handle++));

  // get size before calling set_open_file() because set_open_file() will fail if the file's not initialized
  size = obj->get_size();
  obj->set_open_file(file);

  lock.unlock();

  r = _ft->download(req, obj->get_url(), size, file->get_fd());

  lock.lock();

  if (r)
    obj->set_open_file(open_file::ptr());
  else
    file->set_status(FS_IDLE);

  return r;
}

int open_file_cache::flush(const request_ptr &req, uint64_t handle, bool close_when_done)
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
    r = _ft->upload(req, obj);

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

  r = pwrite(file->obj->get_local_fd(), buffer, size, offset);

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

  r = pread(file->obj->get_local_fd(), buffer, size, offset);

  lock.lock();
  file->status &= ~FS_IN_USE;

  return r;
}

