#include "objects/reference_xattr.h"

using std::runtime_error;
using std::string;

using s3::objects::reference_xattr;

bool reference_xattr::is_serializable()
{
  return false;
}

bool reference_xattr::is_read_only()
{
  return true;
}

void reference_xattr::set_value(const char *value, size_t size)
{
  throw runtime_error("cannot set value of a reference xattr.");
}

int reference_xattr::get_value(char *buffer, size_t max_size)
{
  // see note re. logic in xattr_value::get_value()

  size_t ref_size = 0, size = 0;

  if (_mutex)
    _mutex->lock();

  // terminating nulls are not passed along
  ref_size = _reference->size();

  size = (ref_size > max_size) ? max_size : ref_size;

  if (buffer == NULL) {
    if (_mutex)
      _mutex->unlock();

    return ref_size;
  }

  memcpy(buffer, _reference->c_str(), size);

  if (_mutex)
    _mutex->unlock();

  return (size == ref_size) ? size : -ERANGE;
}

void reference_xattr::to_header(string *header, string *value)
{
  throw runtime_error("cannot serialize by-reference xattrs.");
}
