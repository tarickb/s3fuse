#include "s3_debug.hh"
#include "s3_request_cache.hh"

using namespace boost;
using namespace std;

using namespace s3;

request_cache::request_cache()
{ }

request_cache::~request_cache()
{
  mutex::scoped_lock lock(_mutex);

  for (request_vector::iterator itor = _cache.begin(); itor != _cache.end(); ++itor)
    delete *itor;
}

request_ptr request_cache::get()
{
  mutex::scoped_lock lock(_mutex);
  request *r = NULL;

  for (request_vector::iterator itor = _cache.begin(); itor != _cache.end(); ++itor) {
    if ((*itor)->_ref_count == 0) {
      r = *itor;
      break;
    }
  }

  if (!r) {
    S3_DEBUG("request_cache::get", "no free requests found in cache of size %i.\n", _cache.size());

    r = new request();
    _cache.push_back(r);
  }

  r->reset();
  return request_ptr(r, true); // call add_ref
}

