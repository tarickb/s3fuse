#include "logging.hh"
#include "object_cache.hh"
#include "request.hh"

using namespace s3;

object_cache::object_cache(const thread_pool::ptr &pool)
  : _pool(pool),
    _hits(0),
    _misses(0),
    _expiries(0)
{ }

object_cache::~object_cache()
{
  uint64_t total = _hits + _misses + _expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  S3_DEBUG(
    "object_cache::~object_cache", 
    "hits: %" PRIu64 " (%.02f%%), misses: %" PRIu64 " (%.02f%%), expiries: %" PRIu64 " (%.02f%%)\n", 
    _hits,
    double(_hits) / double(total) * 100.0,
    _misses,
    double(_misses) / double(total) * 100.0,
    _expiries,
    double(_expiries) / double(total) * 100.0);
}

int object_cache::__fetch(const request::ptr &req, const std::string &path, int hints, object::ptr *_obj)
{
  object::ptr &obj = *_obj;

  obj.reset(new object(path));

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
  else
    set(path, obj);

  return 0;
}
