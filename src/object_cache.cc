#include "object_cache.h"
#include "request.h"

using namespace boost;
using namespace std;

using namespace s3;

object_cache::object_cache(
  const thread_pool::ptr &pool, 
  const boost::shared_ptr<mutexes> &mutexes,
  const boost::shared_ptr<file_transfer> &file_transfer)
  : _pool(pool),
    _mutexes(mutexes),
    _file_transfer(file_transfer),
    _hits(0),
    _misses(0),
    _expiries(0),
    _next_handle(0)
{ }

object_cache::~object_cache()
{
  uint64_t total = _hits + _misses + _expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  S3_LOG(
    LOG_DEBUG,
    "object_cache::~object_cache", 
    "hits: %" PRIu64 " (%.02f%%), misses: %" PRIu64 " (%.02f%%), expiries: %" PRIu64 " (%.02f%%)\n", 
    _hits,
    double(_hits) / double(total) * 100.0,
    _misses,
    double(_misses) / double(total) * 100.0,
    _expiries,
    double(_expiries) / double(total) * 100.0);

  if (_next_handle)
    S3_LOG(LOG_DEBUG, "object_cache::~object_cache", "number of opened files: %ju\n", static_cast<uintmax_t>(_next_handle));
}

int object_cache::__fetch(const request::ptr &req, const string &path, int hints, object::ptr *_obj)
{
  object::ptr &obj = *_obj;

  obj.reset(new object(_mutexes, path));

  req->init(HTTP_HEAD);
  req->set_target_object(obj);

  if (hints == HINT_NONE || hints & HINT_IS_DIR) {
    // see if the path is a directory (trailing /) first
    req->set_url(object::build_url(path, OT_DIRECTORY));
    req->run();
  }

  if (hints & HINT_IS_FILE || req->get_response_code() != 200) {
    // it's not a directory
    req->set_url(object::build_url(path, OT_INVALID));
    req->run();
  }

  if (req->get_response_code() != 200)
    obj.reset();
  else {
    mutex::scoped_lock lock(_mutex);
    object::ptr &map_obj = _cache_map[path];

    if (map_obj) {
      // if the object is already in the map, don't overwrite it
      obj = map_obj;
    } else {
      // otherwise, save it
      map_obj = obj;
    }
  }

  return 0;
}
