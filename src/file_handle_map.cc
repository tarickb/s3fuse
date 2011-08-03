#include "file_handle_map.h"

using namespace boost;
using namespace std;

using namespace s3;

int file_handle_map::open(const locked_object::ptr &obj, uint64_t *handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  file *f = NULL;
  open_file::ptr of;
  int r;

  if (obj->get()->get_type() != OT_FILE)
    S3_LOG(LOG_WARNING, "file_handle_map::open", "attempt to open object that isn't a file!\n");
    return -EINVAL;
  }

  f = static_cast<file *>(obj->get().get());

  of = f->open(_next_handle);

  if (of->get_handle() == _next_handle) {
    _map[_next_handle] = obj;
    _next_handle++;
  }

  lock.unlock();
  r = of->init();
  lock.lock();

  if (r) {
    S3_LOG(LOG_WARNING, "file_handle_map::open", "failed to open file [%s] with error %i.\n", f->get_path().c_str(), r);

    f->close();
    _map.erase(handle);
    return r;
  }

  return of->add_reference(handle);
}

int file_handle_map::release(uint64_t handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  handle_map::iterator itor = _map.find(handle);
  file *f = NULL;

  if (itor == _map.end()) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "attempt to release handle not in map.\n");
    return -EINVAL;
  }

  f = static_cast<file *>(itor->second->get().get());

  if (f->open()->release()) {
    _map.erase(handle);

    lock.unlock();
    f->open()->
    obj->get_open_file()->cleanup();
    lock.lock();

    // keep in _cache_map until cleanup() returns so that any concurrent attempts to open the file fail
    _cache_map.erase(obj->get_path());

    // _obj's reference to the open file is deliberately not reset.  we do this to handle the case where 
    // an object pointer has already been returned by get() but has not yet been used.  an attempt to call
    // open_handle() on this zombie object will fail, but metadata calls will still return more-or-less 
    // correct information (except the MD5 sum, for instance).
  }

  return 0;
}

open_file::ptr file_handle_map::get(uint64_t handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  handle_map::const_iterator itor = _handle_map.find(handle);

  return (itor == _handle_map.end()) ? open_file::ptr() : itor->second->get_open_file();
}
