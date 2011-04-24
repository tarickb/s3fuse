#include <sys/time.h>

#include <iostream>
#include <map>
#include <string>
#include <boost/thread.hpp>

#include "logging.hh"

namespace s3
{
  class stats_cache
  {
  public:
    static const int DEFAULT_TIME_TO_LIVE_IN_S = 3 * 60; // 3 minutes

    inline stats_cache(int time_to_live_in_s = DEFAULT_TIME_TO_LIVE_IN_S)
      : _ttl(time_to_live_in_s),
        _hits(0),
        _misses(0),
        _expiries(0)
    { }

    inline ~stats_cache()
    {
      uint64_t total = _hits + _misses + _expiries;

      if (total == 0)
        total = 1; // avoid NaNs below

      S3_DEBUG(
        "stats_cache::~stats_cache", 
        "hits: %lli (%.02f%%), misses: %lli (%.02f%%), expiries: %lli (%.02f%%)\n", 
        _hits,
        double(_hits) / double(total) * 100.0,
        _misses,
        double(_misses) / double(total) * 100.0,
        _expiries,
        double(_expiries) / double(total) * 100.0);
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
