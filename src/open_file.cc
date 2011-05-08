#include "object.hh"
#include "open_file.hh"

using namespace boost;
using namespace std;

using namespace s3;

open_file::open_file(const object::ptr &obj, uint64_t handle)
  : _obj(obj),
    _handle(handle),
    _ref_count(1)
{
  char temp_name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(temp_name);
  _temp_name = temp_name;

  if (_fd == -1)
    throw runtime_error("error calling mkstemp()");

  _status = FS_OPENING;
}

open_file::~open_file()
{
}

int open_file::clone_handle(uint64_t *handle)
{
  if (_status != FS_IDLE)
    return -EBUSY;

  _ref_count++;
  *handle = _handle;
  return 0;
}

int open_file::close()
{
  if (_status != FS_IDLE)
    return -EBUSY;

  _ref_count--;
}
