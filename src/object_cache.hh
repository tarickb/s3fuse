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
      const boost::shared_ptr<object> &ptr = 

      
    }

    inline bool get(const std::string &path, std::string *etag, struct stat *s)
    {
      boost::mutex::scoped_lock lock(_mutex);
      cache_entry &e = _cache[path];

      if (e.expiry == 0) {
        _misses++;
        return false;
      }

      if (e.expiry < time(NULL)) {
        _expiries++;
        return false;
      }

      _hits++;

      if (s)
        memcpy(s, &e.stats, sizeof(*s));

      if (etag)
        *etag = e.etag;

      return true;
    }

    inline void update(const std::string &path, const std::string &etag, const struct stat *s)
    {
      boost::mutex::scoped_lock lock(_mutex);
      cache_entry &e = _cache[path];

      e.expiry = time(NULL) + _ttl;
      e.etag = etag;
      memcpy(&e.stats, s, sizeof(*s));
    }

    inline void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);
      _cache[path] = cache_entry();
    }

  private:
    struct cache_entry
    {
      time_t expiry;
      std::string etag;
      struct stat stats;

      inline cache_entry() : expiry(0) {}
    };

    typedef std::map<std::string, cache_entry> cache_map;

    cache_map _cache;
    boost::mutex _mutex;
    int _ttl;
    uint64_t _hits, _misses, _expiries;
  };
}

#endif
