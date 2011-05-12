#include "file_transfer.hh"
#include "object.hh"
#include "open_file.hh"
#include "open_file_map.hh"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  enum file_status
  {
    FS_READY     = 0x01,
    FS_ZOMBIE    = 0x02,
    FS_FLUSHABLE = 0x04,
    FS_WRITEABLE = 0x08,
    FS_DIRTY     = 0x10
  };
}

open_file::open_file(open_file_map *map, const object::ptr &obj, uint64_t handle)
  : _map(map),
    _obj(obj),
    _handle(handle),
    _ref_count(0),
    _fd(-1),
    _status(0),
    _error(0)
{
  char temp_name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(temp_name);

  // TODO: no unlink in debug mode?
  // unlink(temp_name);

  if (_fd == -1)
    throw runtime_error("error calling mkstemp()");
}

open_file::~open_file()
{
  close(_fd);
}

int open_file::init()
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (_status & FS_READY)
    return -EINVAL;

  lock.unlock();
  r = _map->get_file_transfer()->download(_obj->get_url(), _obj->get_size(), _fd);
  lock.lock();

  if (r)
    _error = r;

  _status |= FS_READY | FS_FLUSHABLE | FS_WRITEABLE;
  _map->get_file_status_condition().notify_all();

  return r;
}

int open_file::cleanup()
{
  if (!(_status & FS_ZOMBIE))
    return -EINVAL;

  return flush();
}

int open_file::add_reference(uint64_t *handle)
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());

  if (_status & FS_ZOMBIE)
    return -EINVAL;

  if (!(_status & FS_READY)) {
    while (!(_status & FS_READY))
      _map->get_file_status_condition().wait(lock);

    if (_error)
      return _error;
  }

  *handle = _handle;
  _ref_count++;

  return 0;
}

bool open_file::release()
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());

  if (_ref_count == 0)
    return -EINVAL;

  _ref_count--;

  if (_ref_count == 0)
    _status |= FS_ZOMBIE;

  return _status & FS_ZOMBIE;
}

int open_file::flush()
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (!(_status & FS_DIRTY))
    return 0;

  // force flush in zombie state even if not flushable
  if (!(_status & FS_FLUSHABLE) && !(_status & FS_ZOMBIE))
    return -EBUSY;

  _status &= ~(FS_FLUSHABLE | FS_WRITEABLE);

  lock.unlock();
  r = _map->get_file_transfer()->upload(_obj, _fd);
  lock.lock();

  if (r == 0)
    _status &= ~FS_DIRTY;

  _status |= FS_FLUSHABLE | FS_WRITEABLE;

  return r;
}

int open_file::write(const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (!(_status & FS_READY) || !(_status & FS_WRITEABLE))
    return -EBUSY;

  _status &= ~FS_FLUSHABLE;

  lock.unlock();
  r = pwrite(_fd, buffer, size, offset);
  lock.lock();

  _status |= FS_FLUSHABLE | FS_DIRTY;

  return r;
}

int open_file::read(char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());

  if (!(_status & FS_READY))
    return -EBUSY;

  lock.unlock();

  return pread(_fd, buffer, size, offset);
}

