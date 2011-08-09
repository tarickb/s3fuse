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
  locked_object::ptr lo = _object_map[get_object(obj)];

  // TODO: audit code for map references that may not remain stable

  if (lo && !get_object(lo)->is_in_use()) {
    // counterintuitive, perhaps, to return -EBUSY here but if is_in_use() is 
    // false, then the object has yet to be initialized.  that would suggest 
    // that another thread is still in open().

    S3_LOG(LOG_WARNING, "file_handle_map::open", "object [%s] is busy!\n", obj->get()->get_path().c_str());
    return -EBUSY;
  }

  if (!lo) {
    object_handle new_handle;
    int r;

    lo = obj;

    if (get_object(lo)->get_type() != OT_FILE) {
      S3_LOG(LOG_WARNING, "file_handle_map::open", "attempt to open object that isn't a file!\n");
      return -EINVAL;
    }

    new_handle = _next_handle++;
    get_object(lo)->set_handle(new_handle);
    _object_map[get_object(lo)] = lo;

    // subsequent attempts to open() or close() this file will fail with -EBUSY
    lock.unlock();
    r = get_file(lo)->open();
    lock.lock();

    if (r) {
      S3_LOG(LOG_WARNING, "file_handle_map::open", "failed to open file [%s] with error %i.\n", get_object(lo)->get_path().c_str(), r);
      _object_map.erase(get_object(lo));

      return r;
    }

    _handle_map[new_handle] = lo;
  }

  get_object(lo)->add_ref(handle);

  return 0;
}

int file_handle_map::release(object_handle handle)
{
  boost::mutex::scoped_lock lock(_mutex);
  handle_map::iterator itor = _handle_map.find(handle);
  locked_object::ptr lo;
  int r = 0;

  if (itor == _handle_map.end()) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "attempt to release handle not in map.\n");
    return -EINVAL;
  }

  lo = itor->second;

  if (!lo) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "handle is in map, but pointer is invalid.\n");
    return -EINVAL;
  }

  if (!get_object(lo)->is_in_use()) {
    S3_LOG(LOG_WARNING, "file_handle_map::release", "attempt to close handle that's being opened/closed elsewhere.\n");
    return -EBUSY;
  }

  get_object(lo)->release();

  if (!get_object(lo)->is_in_use()) {
    // subsequent calls to open() or close() will return -EBUSY
    lock.unlock();
    r = get_file(lo)->close();
    lock.lock();

    _handle_map.erase(itor);
    _object_map.erase(get_object(lo));
  }

  return r;
}

