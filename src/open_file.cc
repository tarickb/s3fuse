#include "file_transfer.hh"
#include "logging.hh"
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

  S3_DEBUG("open_file::open_file", "opening [%s] in [%s].\n", obj->get_path().c_str(), temp_name);

  // TODO: no unlink in debug mode?
  // unlink(temp_name);

  if (_fd == -1)
    throw runtime_error("error calling mkstemp()");

  if (ftruncate(_fd, obj->get_size()) != 0)
    throw runtime_error("failed to truncate temporary file.");
}

open_file::~open_file()
{
  S3_DEBUG("open_file::~open_file", "closing temporary file for [%s].\n", _obj->get_path().c_str());
  close(_fd);
}

int open_file::init()
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (_status & FS_READY) {
    S3_DEBUG("open_file::init", "attempt to init file with FS_READY set for [%s].\n", _obj->get_path().c_str());
    return -EINVAL;
  }

  lock.unlock();
  r = _map->get_file_transfer()->download(_obj->get_url(), _obj->get_size(), _fd);
  lock.lock();

  if (r)
    _error = r;

  S3_DEBUG("open_file::init", "file [%s] ready.\n", _obj->get_path().c_str());

  _status |= FS_READY | FS_FLUSHABLE | FS_WRITEABLE;
  _map->get_file_status_condition().notify_all();

  return r;
}

int open_file::cleanup()
{
  if (!(_status & FS_ZOMBIE)) {
    S3_DEBUG("open_file::cleanup", "attempt to clean up file with FS_ZOMBIE not set for [%s].\n", _obj->get_path().c_str());
    return -EINVAL;
  }

  return flush();
}

int open_file::add_reference(uint64_t *handle)
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());

  if (_status & FS_ZOMBIE) {
    S3_DEBUG("open_file::add_reference", "attempt to add reference for file with FS_ZOMBIE set for [%s].\n", _obj->get_path().c_str());
    return -EINVAL;
  }

  if (!(_status & FS_READY)) {
    S3_DEBUG("open_file::add_reference", "file [%s] not yet ready. waiting.\n", _obj->get_path().c_str());

    while (!(_status & FS_READY))
      _map->get_file_status_condition().wait(lock);

    S3_DEBUG("open_file::add_reference", "done waiting for [%s]. error: %i.\n", _obj->get_path().c_str(), _error);

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

  if (_ref_count == 0) {
    S3_DEBUG("open_file::release", "attempt to release handle on [%s] with zero ref-count.\n", _obj->get_path().c_str());
    return -EINVAL;
  }

  _ref_count--;

  if (_ref_count == 0) {
    S3_DEBUG("open_file::release", "file [%s] is now a zombie.\n", _obj->get_path().c_str());
    _status |= FS_ZOMBIE;
  }

  return _status & FS_ZOMBIE;
}

int open_file::flush()
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (!(_status & FS_DIRTY)) {
    S3_DEBUG("open_file::flush", "skipping flush for file [%s].\n", _obj->get_path().c_str());
    return 0;
  }

  // force flush in zombie state even if not flushable
  if (!(_status & FS_FLUSHABLE) && !(_status & FS_ZOMBIE)) {
    S3_DEBUG("open_file::flush", "failing concurrent flush call for [%s].\n", _obj->get_path().c_str());
    return -EBUSY;
  }

  _status &= ~(FS_FLUSHABLE | FS_WRITEABLE);

  lock.unlock();
  r = _map->get_file_transfer()->upload(_obj, _fd);
  lock.lock();

  if (r == 0)
    _status &= ~FS_DIRTY;
  else
    S3_DEBUG("open_file::flush", "failed to upload [%s] with error %i.\n", _obj->get_path().c_str(), r);

  _status |= FS_FLUSHABLE | FS_WRITEABLE;

  return r;
}

int open_file::write(const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_map->get_file_status_mutex());
  int r;

  if (!(_status & FS_READY) || !(_status & FS_WRITEABLE)) {
    S3_DEBUG("open_file::write", "failing write attempt on [%s] with status %i.\n", _obj->get_path().c_str(), _status);
    return -EBUSY;
  }

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

  if (!(_status & FS_READY)) {
    S3_DEBUG("open_file::read", "read on [%s] when file isn't ready.\n", _obj->get_path().c_str());
    return -EBUSY;
  }

  lock.unlock();

  return pread(_fd, buffer, size, offset);
}

