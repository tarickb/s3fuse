#ifndef S3_OBJECT_CACHE_HH
#define S3_OBJECT_CACHE_HH

#include <sys/time.h>

#include <iostream>
#include <map>
#include <string>
#include <boost/thread.hpp>

#ifndef S3_LOGGING_HH
  #error include logging.hh before this file.
#endif

namespace s3
{
  class object_cache
  {
  public:
    inline object_cache()
      : _hits(0),
        _misses(0),
        _expiries(0)
    { }

    inline ~object_cache()
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

    inline const boost::shared_ptr<object> & get(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);
      const boost::shared_ptr<object> &object = _cache[path];

      if (!object)
        _misses++;
      else if (!object->is_valid())
        _expiries++;
      else
        _hits++;

      return object;
    }

    inline void set(const std::string &path, const boost::shared_ptr<object> &object)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _cache[path] = object;
    }

    inline void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _cache.erase(path);
    }

  private:
    typedef boost::shared_ptr<object> object_ptr;
    typedef std::map<std::string, object_ptr> cache_map;

    cache_map _cache;
    boost::mutex _mutex;
    uint64_t _hits, _misses, _expiries;
  };
}

#endif
