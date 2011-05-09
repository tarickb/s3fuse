#include "file_transfer.hh"
#include "object.hh"
#include "open_file.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  enum file_status
  {
    FS_INIT     = 0x00,
    FS_OPENING  = 0x01,
    FS_OPEN     = 0x02,
    FS_CLOSING  = 0x04,
    FS_IN_USE   = 0x08,
    FS_FLUSHING = 0x10,
    FS_DIRTY    = 0x20
  }
}

open_file::open_file(boost::mutex *mutex, const file_transfer::ptr &ft, const object::ptr &obj, uint64_t handle)
  : _mutex(mutex),
    _ft(ft),
    _obj(obj),
    _handle(handle),
    _ref_count(1),
    _status(0)
{
  char temp_name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(temp_name);
  _temp_name = temp_name;

  if (_fd == -1)
    throw runtime_error("error calling mkstemp()");
}

open_file::~open_file()
{
  cloae();
}

void open_file::close()
{
  if (_status 
  close(_fd);
  unlink(_temp_name.c_str());
}

int open_file::open()
{
  mutex::scoped_lock lock(*_mutex);
  int r;

  if (_status)
    return -EBUSY;

  _status = FS_IN_USE;
  lock.unlock();

  r = _ft->download(_obj->get_url(), _obj->get_size(), _fd);

  lock.lock();
  _status = r ? 0 : FS_READY;

  return r;
}

int open_file::clone_handle(uint64_t *handle)
{
  mutex::scoped_lock lock(*_mutex);

  if (_ref_count == 0)
    return -EINVAL;

  _ref_count++;
  *handle = _handle;

  return 0;
}

int open_file::release()
{
  mutex::scoped_lock lock(*_mutex);

  if (_ref_count == 0)
    return -EINVAL;

  _ref_count--;

  if (_ref_count == 0)
    close();

  return 0;
}

int open_file::flush(const request_ptr &req)
{
  mutex::scoped_lock lock(*_mutex);
  int r;

  if (_status & FS_IN_USE || !(_status & FS_READY))
    return -EBUSY;

  if (_status & FS_FLUSHING || !(_status & FS_DIRTY))
    return 0;

  _status |= FS_FLUSHING;
  lock.unlock();

  r = _ft->upload(req, _obj);

  lock.lock();
  file->status &= ~FS_FLUSHING;

  if (!r)
    file->status &= ~FS_DIRTY;

  return r;
}

int open_file::write(const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(*_mutex);
  int r;

  if (!_ref_count)
    return -EINVAL;

  if (_status & FS_FLUSHING || _status & FS_OPENING)
    return -EBUSY;

  _status |= FS_IN_USE;
  lock.unlock();

  r = pwrite(_fd, buffer, size, offset);

  lock.lock();
  _status &= ~FS_IN_USE;
  _status |= FS_DIRTY;

  return r;
}

int open_file::read(char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(*_mutex);
  int r;

  if (_status & FS_FLUSHING || _status & FS_OPENING)
    return -EBUSY;

  _status |= FS_IN_USE;
  lock.unlock();

  r = pread(_fd, buffer, size, offset);

  lock.lock();
  _status &= ~FS_IN_USE;

  return r;
}

