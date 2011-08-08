#include "file_handle_map.h"
#include "logger.h"

using namespace boost;
using namespace std;

using namespace s3;

file_handle_map::file_handle_map()
  : _next_handle(0)
{
}

int file_handle_map::open(const locked_object::ptr &obj, object_handle *handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  handle_container::ptr hc = _object_map[obj->get().get()];

  // TODO: audit code for map references that may not remain stable

  if (hc && !hc->is_in_use()) {
    // counterintuitive, perhaps, to return -EBUSY here but if is_in_use() is 
    // false, then the object has yet to be initialized.  that would suggest 
    // that another thread is still in open().

    S3_LOG(LOG_WARNING, "file_handle_map::open", "object [%s] is busy!\n", obj->get()->get_path().c_str());
    return -EBUSY;
  }

  if (!hc) {
    object_handle new_handle;
    int r;

    if (obj->get()->get_type() != OT_FILE) {
      S3_LOG(LOG_WARNING, "file_handle_map::open", "attempt to open object that isn't a file!\n");
      return -EINVAL;
    }

    new_handle = _next_handle++;
    hc.reset(new handle_container(obj, new_handle));
    _object_map[hc->get_object()] = hc;

    // subsequent attempts to open() or close() this file will fail with -EBUSY
    lock.unlock();
    r = hc->get_file()->open();
    lock.lock();

    if (r) {
      S3_LOG(LOG_WARNING, "file_handle_map::open", "failed to open file [%s] with error %i.\n", obj->get()->get_path().c_str(), r);
      _object_map.erase(hc->get_object());

      return r;
    }

    _handle_map[new_handle] = hc;
  }

  hc->add_ref(handle);

  return 0;
}

int file_handle_map::release(object_handle handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  handle_map::iterator itor = _handle_map.find(handle);
  handle_container::ptr hc;
  int r = 0;

  if (itor == _handle_map.end()) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "attempt to release handle not in map.\n");
    return -EINVAL;
  }

  hc = itor->second;

  if (!hc) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "handle is in map, but pointer is invalid.\n");
    return -EINVAL;
  }

  if (!hc->is_in_use()) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "attempt to close handle that's being opened/closed elsewhere.\n");
    return -EBUSY;
  }

  hc->release();

  if (!hc->is_in_use()) {
    // subsequent calls to open() or close() will return -EBUSY
    lock.unlock();
    r = hc->get_file()->close();
    lock.lock();

    _handle_map.erase(itor);
    _object_map.erase(hc->get_object());
  }

  return r;
}

